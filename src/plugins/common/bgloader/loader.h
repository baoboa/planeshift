/*
 * loader.h - Author: Mike Gist
 *
 * Copyright (C) 2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef __LOADER_H__
#define __LOADER_H__

#include <csgeom/poly3d.h>
#include <csgfx/shadervar.h>
#include <csutil/scf_implementation.h>
#include <csutil/hash.h>
#include <csutil/threading/rwmutex.h>
#include <csutil/threadmanager.h>
#include <csutil/refcount.h>
#include <csutil/typetraits.h>

#include <iengine/engine.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/meshgen.h>
#include <iengine/sector.h>
#include <iengine/texture.h>
#include <iengine/movable.h>
#include <imesh/object.h>
#include <imesh/objmodel.h>
#include <imap/loader.h>
#include <iutil/objreg.h>
#include <iutil/vfs.h>

#include <ibgloader.h>
#include <iscenemanipulate.h>

#ifdef CS_DEBUG
#define LOADER_DEBUG_MESSAGE(...) csPrintf(__VA_ARGS__)
#else
#define LOADER_DEBUG_MESSAGE(...)
#endif

//#ifdef CS_DEBUG
#undef  CS_ASSERT_MSG
#define CS_ASSERT_MSG(msg, x) if(!(x)) printf("ART ERROR: %s\n", msg)
//#endif

struct iCollideSystem;
struct iEngineSequenceManager;
struct iSyntaxService;

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{

// string literals for usage as template parameter
namespace ObjectNames
{
    extern const char texture[8];
    extern const char material[9];
    extern const char trigger[8];
    extern const char sequence[9];
    extern const char meshobj[5];
    extern const char meshfact[13];
    extern const char meshgen[8];
    extern const char light[6];
    extern const char portal[7];
    extern const char sector[7];
}

/**
 * The BgLoader is a wrapper around the Crystalspace iThreadedLoader interface.
 * The main idea is to read all the files that iThreadedLoader would read
 * but instead of creating the iEngine objects, the XML information is saved
 * and used later to create the iEngine objects. The iEngine objects are
 * created when the player's character is moved across sectors or comes within
 * visible range of an object. Thus, the BgLoader can load and unload
 * iEngine objects as needed, saving memory and load time.
 */
class BgLoader : public ThreadedCallable<BgLoader>,
                 public scfImplementation3<BgLoader,
                                           iBgLoader,
                                           iSceneManipulate,
                                           iComponent>
{
private:
    // forward declaration
    class Loadable;
    struct iDelayedLoader;

public:
    BgLoader(iBase *p);
    virtual ~BgLoader();

   /**
    * Plugin initialisation.
    */
    bool Initialize(iObjectRegistry* _object_reg);

   /**
    * Start loading a material into the engine. Check return for success once it's finished.
    */
    csPtr<iThreadReturn> LoadMaterial(const char* name, bool wait = false);

   /**
    * Start loading a mesh factory into the engine. Check return for success once it's finished.
    */
    csPtr<iThreadReturn> LoadFactory(const char* name, bool wait = false);

    /**
    * Clone a mesh factory.
    * @param name The name of the mesh factory to clone.
    * @param newName The name of the new cloned mesh factory.
    * @param load Begin loading the cloned mesh factory.
    * @param failed Pass a boolean to be able to manually handle a failed clone.
    * @param scale pass a transform in here that should be applied to the factory
    */
    void CloneFactory(const char* name, const char* newName, bool* failed = NULL, const csReversibleTransform& trans = csReversibleTransform());

   /**
    * Pass a data file to be cached. This method will parse your data and add it to it's
    * internal world representation. You may then request that these objects are loaded.
    * This call will be dispatched to a thread, so it will return immediately.
    * You should wait for parsing to finish before calling UpdatePosition().
    */
    THREADED_CALLABLE_DECL1(BgLoader, PrecacheData, csThreadReturn, const char*, path, THREADEDL, false, false);

   /**
    * Clears all temporary data that is only required parse time.
    * calls to PrecacheData mustn't occur after this function has been called
    */
    void ClearTemporaryData()
    {
        parserData.xmltokens.Empty();
        parserData.textures.Clear();
        parserData.meshes.Clear();
        parserData.svstrings.Invalidate();
        parserData.syntaxService.Invalidate();
        tman.Invalidate();
    }

   /**
    * Update your position in the world.
    * Calling this will trigger per-object checks and initiate (un)loading if the object
    * is within a given threshold (loadRange).
    * @param pos Your world space position.
    * @param sectorName The name of the sector that you are currently in.
    * @param force Forces the checks to be done (normally they won't if you e.g. haven't moved).
    */
    void UpdatePosition(const csVector3& pos, const char* sectorName, bool force);

   /**
    * Call this function to finalise a number of loading objects.
    * Useful when you are waiting for a load to finish (load into the world, teleport),
    * but want to continue rendering while you wait.
    * Will return after processing a number of objects.
    * @param waiting Set as 'true' if you're waiting for the load to finish.
    * This will make it process more before returning (lower overhead).
    * Note that you should process a frame after each call, as spawned threads
    * may depend on the main thread to handle requests.
    */
    void ContinueLoading(bool waiting);

   /**
    * Returns a pointer to the Crystal Space threaded loader.
    */
    iThreadedLoader* GetLoader() { return tloader; }

   /**
    * Returns the number of objects currently loading.
    */
    size_t GetLoadingCount() { return loadList.GetSize(); }

   /**
    * Returns a pointer to the object registry.
    */
    iObjectRegistry* GetObjectRegistry() const { return object_reg; }

   /**
    * Update the load range initially passed to the loader in Setup().
    */
    void SetLoadRange(float r) { loadRange = r; if (lastSector.IsValid()) UpdatePosition(lastPos, lastSector->GetName(), true); }

   /**
    * Request to know whether the current world position stored by the loader is valid.
    * Returns false until the first call of UpdatePosition().
    */
    bool HasValidPosition() const { return validPosition; }

   /**
    * Request to know whether you are currently positioned in a water body.
    * @param sector The sector that you are checking.
    * @param pos The world space position that you are checking.
    * @param colour Will contain the colour of the water that you are positioned in.
    */
    bool InWaterArea(const char* sector, csVector3* pos, csColor4** colour);

   /**
    * Load zones given by name.
    */
    bool LoadZones(iStringArray* regions, bool priority = false);

   /**
    * Load high priority zones given by name.
    * High priority zones are not unloaded by UpdatePosition or LoadZones.
    */
    bool LoadPriorityZones(iStringArray* regions);

   /**
    * Returns an array of the available shaders for a given type.
    * @param usageType The type of shader you wish to have.
    * E.g. 'default_alpha' to get an array of all default world alpha shaders.
    */
    csPtr<iStringArray> GetShaderName(const char* usageType);

   /**
    * Creates a new instance of the given factory at the given screen space coordinates.
    * @param factName The name of the factory to be used to create the mesh.
    * @param matName The optional name of the material to set on the mesh. Pass NULL to set none.
    * @param camera The camera related to the screen space coordinates.
    * @param pos The screen space coordinates.
    */
    iMeshWrapper* CreateAndSelectMesh(const char* factName, const char* matName,
        iCamera* camera, const csVector2& pos);

   /**
    * Selects the closest mesh at the given screen space coordinates.
    * @param camera The camera related to the screen space coordinates.
    * @param pos The screen space coordinates.
    */
    iMeshWrapper* SelectMesh(iCamera* camera, const csVector2& pos);

   /**
    * Translates the mesh selected by CreateAndSelectMesh() or SelectMesh().
    * @param vertical True if you want to translate vertically (along y-axis).
    * False to translate snapped to the mesh at the screen space coordinates.
    * @param camera The camera related to the screen space coordinates.
    * @param pos The screen space coordinates.
    */
    bool TranslateSelected(bool vertical, iCamera* camera, const csVector2& pos);

   /**
    * Rotates the mesh selected by CreateAndSelectMesh() or SelectMesh().
    * @param pos The screen space coordinates to use to base the rotation, relative to the last saved coordinates.
    */
    void RotateSelected(const csVector2& pos);

   /**
    * Set the axes to rotate around with RotateSelected.
    * @param flags_h bitflag with axes for horizontal mouse movement
    * @param flags_v bitflag with axes for vertical mouse movement
    * @see PS_MANIPULATE
    */
    void SetRotation(int flags_h, int flags_v);

   /**
    * Sets the previous position (e.g. in case you warped the mouse)
    */
    void SetPosition(const csVector2& pos) { previousPosition = pos; };

   /**
    * Removes the currently selected mesh from the scene.
    */
    void RemoveSelected();

    void GetPosition(csVector3 & pos, csVector3 & rot, const csVector2& screenPos);

   /**
    * Returns an array of start positions in the world.
    */
    csRefArray<StartPosition> GetStartPositions()
    {
        csRefArray<StartPosition> array;
        CS::Threading::ScopedReadLock lock(parserData.positions.lock);
        LockedType<StartPosition>::HashType::GlobalIterator it(parserData.positions.hash.GetIterator());
        while(it.HasNext())
        {
            array.Push(it.Next());
        }
        return array;
    }

    // internal accessors
    iEngine* GetEngine() const
    {
        return engine;
    }

    iVFS* GetVFS()
    {
        vfsLock.Lock();
        return vfs;
    }

    void ReleaseVFS()
    {
        vfsLock.Unlock();
    }

    iThreadedLoader* GetLoader() const
    {
        return tloader;
    }
    
    iCollideSystem* GetCDSys() const
    {
        return cdsys;
    }

    // increase load count - to be used by loadables only
    void RegisterPendingObject(Loadable* obj)
    {
        CS::Threading::RecursiveMutexScopedLock lock(loadLock);
        loadList.Push(obj);
    }

    // decrease load count - to be used by loadables only
    void UnregisterPendingObject(Loadable* obj)
    {
        CS::Threading::RecursiveMutexScopedLock lock(loadLock);
        size_t index = loadList.Find(obj);

        // make sure ContinuedLoading won't skip any elements
        if(index <= loadOffset)
        {
            --loadOffset;
        }
        loadList.DeleteIndex(index);
    }

    // register delayed loader
    void RegisterDelayedLoader(iDelayedLoader* loader)
    {
        CS::Threading::RecursiveMutexScopedLock lock(loadLock);
        delayedLoadList.Push(loader);
    }

    void UnregisterDelayedLoader(iDelayedLoader* loader)
    {
        CS::Threading::RecursiveMutexScopedLock lock(loadLock);
        size_t index = delayedLoadList.Find(loader);

        // make sure ContinuedLoading won't skip any elements
        if(index <= delayedOffset)
        {
            --delayedOffset;
        }
        delayedLoadList.DeleteIndex(index);
    }

private:
    // The various gfx feature options we have.
    enum gfxFeatures
    {
      useLowestShaders = 0x1,
      useLowShaders = 0x2 | useLowestShaders,
      useMediumShaders = 0x4 | useLowShaders,
      useHighShaders = 0x8 | useMediumShaders,
      useHighestShaders = 0x10 | useHighShaders,
      useShadows = 0x20,
      useMeshGen = 0x40,
      useAll = (useHighestShaders | useShadows | useMeshGen)
    };

    /********************************************************
     * Data structures representing components of the world.
     *******************************************************/

    // forward declarations
    class Sector;
    class Zone;
    class Texture;
    class MeshObj;
    class Sequence;
    class Light;
    struct ParserData;
    struct GlobalParserData;

    // generic objects used for loading/parsing

    // helper clases used with object classes that represent the world
    template<typename T> struct CheckedLoad
    {
        csRef<T> obj;
        bool checked;

        CheckedLoad(const csRef<T>& obj) : obj(obj), checked(false)
        {
        }

        CheckedLoad(const CheckedLoad& other) : obj(other.obj), checked(false)
        {
        }
    };

    /**
     * LockedType is used to store BgLoader objects.
     */
    template<typename T, bool check = true> struct LockedType
    {
    public:
        typedef csHash<csRef<T>, csStringID> HashType;
        HashType hash;
        csStringSet stringSet;
        CS::Threading::ReadWriteMutex lock;

        csPtr<T> Get(const char* name)
        {
            csRef<T> object;
            CS::Threading::ScopedReadLock scopedLock(lock);
            if(stringSet.Contains(name))
            {
                csStringID objectID = stringSet.Request(name);
                object = hash.Get(objectID, csRef<T>());
            }
            return csPtr<T>(object);
        }

        // workaround for completely braindead gcc 4.0.x (thanks apple!)
        void Put(const csRef<T>& obj)
        {
            Put(obj, 0);
        }

        void Put(const csRef<T>& obj, const char* name)
        {
            CS::Threading::ScopedWriteLock scopedLock(lock);
            csStringID objectID;
            if(name)
            {
                objectID = stringSet.Request(name);
            }
            else
            {
                objectID = stringSet.Request(obj->GetName());
            }

            // check for duplicates
            if(check && hash.Contains(objectID))
            {
                //LOADER_DEBUG_MESSAGE("detected name conflict for object '%s'\n", name ? name : obj->GetName());
            }
            else
            {
                hash.Put(objectID, obj);
            }
        }

        void Delete(const char* name)
        {
            CS::Threading::ScopedWriteLock scopedLock(lock);
            if(stringSet.Contains(name))
            {
                csStringID id = stringSet.Request(name);
                hash.DeleteAll(id);
                stringSet.Delete(id);
            }
        }

        void Clear()
        {
            stringSet.Empty();
            hash.Empty();
        }
    };

    class RangeBased
    {
    protected:
        csBox3 bbox;

    public:
        inline bool InRange(const csBox3& curBBox) const
        {
            return curBBox.Overlap(bbox);
        }

        inline bool OutOfRange(const csBox3& curBBox) const
        {
            return !curBBox.Overlap(bbox);
        }
    };

    // Loaded unconditionally - not range based.
    class AlwaysLoaded
    {
    public:
        inline bool InRange(const csBox3& /*curBBox*/) const
        {
            return true;
        }

        inline bool OutOfRange(const csBox3& /*curBBox*/) const
        {
            return false;
        }
    };

    /**
     * Base class for BgLoader objects.
     */
    class Loadable : public csObject
    {
    public:
        Loadable(BgLoader* parent) : parent(parent),
                                     loading(false), useCount(0)
        {
        }

        Loadable(const Loadable& other) : csObject(0), parent(other.parent),
                                          loading(false), useCount(0)
        {
            SetName(other.GetName());
        }

        virtual ~Loadable()
        {
        }

        /*
         * Return true if underlying CS object was loaded.
         */
        bool Load(bool wait = false)
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            checked = true;
            if(useCount == 0)
            {
                lingerCount = 0;

                if(!loading)
                {
                    loading = true;
                    GetParent()->RegisterPendingObject(this);
                }

                if(LoadObject(wait))
                {
                    loading = false;
                    ++useCount;
                    GetParent()->UnregisterPendingObject(this);

                    FinishObject();
                }

                return !loading;
            }
            else
            {
                ++useCount;
                return true;
            }
        }

        void AbortLoad()
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            if(loading)
            {
                loading = false;
                checked = false;
                
                // do a blocked load and then unload for now
                // to prevent some race conditions with CS' internal loader
                Load(true);
                Unload();

                //UnloadObject();
                GetParent()->UnregisterPendingObject(this);
            }
        }

        void Unload()
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            if(useCount == 0)
            {
                LOADER_DEBUG_MESSAGE("tried to free not loaded object '%s'\n", GetName());
                return;
            }

            CS_ASSERT_MSG("unloading currently loading object!", !loading);

            --useCount;
            if(useCount == 0)
            {
                UnloadObject();
            }
        }

        virtual bool LoadObject(bool wait) = 0;
        virtual void UnloadObject() = 0;
        virtual void FinishObject() {}

        bool IsLoaded() const
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            return useCount > 0;
        }

        inline BgLoader* GetParent() const
        {
            return parent;
        }

        void ResetChecked()
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            checked = false;
        }

        bool IsChecked() const
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            return checked;
        }

        size_t GetLingerCount() const
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            return lingerCount;
        }

        void IncLingerCount()
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            ++lingerCount;
        }

    protected:
        void MarkChecked()
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            checked = true;
        }

        template<typename T, const char* TypeName> void CheckRemove(csRef<T>& ref)
        {
            if(ref.IsValid())
            {
                csWeakRef<T> check(ref);
                parent->GetEngine()->RemoveObject(ref);
                ref.Invalidate();
                if(check.IsValid())
                {
                    LOADER_DEBUG_MESSAGE("detected leaking %s: %u %s\n", TypeName, check->GetRefCount(), GetName());
                }
            }
        }

    private:
        mutable CS::Threading::RecursiveMutex busy;

        BgLoader* parent;
        bool loading;
        bool checked;
        size_t lingerCount;
        size_t useCount;
    };

    struct iDelayedLoader : public iThreadReturn
    {
        virtual void ContinueLoading(bool wait) = 0;
    };

    template<typename T> class DelayedLoader : public scfImplementation1<DelayedLoader<T>, iDelayedLoader>
    {
    public:
        DelayedLoader(T* target) : scfImplementation1<DelayedLoader<T>, iDelayedLoader>(this),
                                   target(target), finished(false),
                                   waitLock(0), waitCondition(0)
        {
            if(target)
            {
                // valid target, register us with the parent
                BgLoader* parent = target->GetParent();
                parent->RegisterDelayedLoader(this);
            }
            else
            {
                finished = true;
            }
        }

        virtual ~DelayedLoader()
        {
            if(!target.IsValid())
            {
                return;
            }

            // free result
            result.Invalidate();

            if(finished)
            {
                // unload the object
                target->Unload(); 
            }
            else
            {
                // unregister us with the parent
                BgLoader* parent = target->GetParent();
                parent->UnregisterDelayedLoader(this);

                // don't abort load here - we're still on the load
                // list so we'll be detected as lingering if applicaple
                // and removed accordingly
            }
        }

        void ContinueLoading(bool wait)
        {
            CS::Threading::MutexScopedLock lock(busy);
            if(target.IsValid() && !finished && target->Load(wait))
            {
                // done loading

                // unregister us with the parent
                BgLoader* parent = target->GetParent();
                parent->UnregisterDelayedLoader(this);

                if(waitLock) waitLock->Lock();

                // mark finished and set result accordingly;
                finished = true;
                csRef<typename T::ObjectType> ref = target->GetObject();
                result = ref;

                // notify tm if it's waiting for us
                if(waitCondition)
                {
                    waitCondition->NotifyAll();
                }

                if(waitLock) waitLock->Unlock();
            }
        }

        void Wait(bool /*process*/)
        {
            ContinueLoading(true);
        }

        void* GetResultPtr()
        {
            CS::Threading::MutexScopedLock lock(busy);
            return result;
        }

        csRef<iBase> GetResultRefPtr()
        {
            CS::Threading::MutexScopedLock lock(busy);
            return result;
        }

        bool IsFinished()
        {
            CS::Threading::MutexScopedLock lock(busy);
            return finished;
        }

        bool WasSuccessful()
        {
            CS::Threading::MutexScopedLock lock(busy);
            return result.IsValid();
        }

        void SetWaitPtrs(CS::Threading::Condition* c, CS::Threading::Mutex* m)
        {
            CS::Threading::MutexScopedLock lock(busy);
            waitCondition = c;
            waitLock = m;
        }

        // doesn't have to be threadsafe
        void SetJob(iJob* newJob)
        {
            job = newJob;
        }

        // doesn't have to be threadsafe
        iJob* GetJob() const
        {
            return job;
        }

    private:
        // disable functions iThreadReturn requires but we don't want to implement
        void MarkSuccessful() {}
        void MarkFinished() {}
        void SetResult(csRef<iBase> /*result*/) {}
        void SetResult(void* /*result*/) {}
        void Copy(iThreadReturn* /*other*/) {}

        // private data
        CS::Threading::Mutex busy;
        csRef<T> target;
        csRef<iBase> result;
        bool finished;

        // thread return specific data
        CS::Threading::Mutex* waitLock;
        CS::Threading::Condition* waitCondition;
        csRef<iJob> job;
    };

    // trivial loadable (e.g. triggers, textures, ...)
    template<typename T, const char* TypeName> class TrivialLoadable : public Loadable
    {
    public:
        typedef T ObjectType;

        TrivialLoadable(BgLoader* parent) : Loadable(parent)
        {
        }

        TrivialLoadable(const TrivialLoadable& other)
            : Loadable(other.GetParent()), path(other.path),
              data(other.data)
        {
        }

        bool LoadObject(bool wait)
        {
            if(!status.IsValid())
            {
                if(!data.IsValid())
                {
                    LOADER_DEBUG_MESSAGE("%s %s was created, but never parsed!\n", TypeName, GetName());
                    return true;
                }
                status = GetParent()->GetLoader()->LoadNode(path, data);
            }

            if(wait)
            {
                status->Wait();
            }

            if(status->IsFinished())
            {
                if(!status->WasSuccessful())
                {
                    LOADER_DEBUG_MESSAGE("%s %s failed to load\n", TypeName, GetName());
                }
                return true;
            }
            else
            {
                return false;
            }
        }

        void UnloadObject()
        {
            csRef<T> ref = GetObject();
            status.Invalidate();
            Loadable::CheckRemove<T,TypeName>(ref);
        }

        csPtr<T> GetObject()
        {
            if(status.IsValid() && status->IsFinished() && status->WasSuccessful())
            {
                csRef<iBase> rawObj = status->GetResultRefPtr();
                if(rawObj.IsValid())
                {
                    return scfQueryInterface<T>(rawObj);
                }
            }

            return csPtr<T>(0);
        }

    protected:
        // parse results
        csString path;
        csRef<iDocumentNode> data;

        // load data
        csRef<iThreadReturn> status;
    };

    // helper class that allows a specific dependency type for an object
    template<typename T> class ObjectLoader
    {
    public:
        typedef CheckedLoad<T> HashObjectType;
        typedef csHash<HashObjectType, csString> HashType;

        ObjectLoader() : objectCount(0)
        {
        }

        ObjectLoader(const ObjectLoader& other) : objectCount(0)
        {
            CS::Threading::RecursiveMutexScopedLock lock(other.busy);
            typename HashType::ConstGlobalIterator it(other.objects.GetIterator());
            while(it.HasNext())
            {
                csString key;
                HashObjectType obj(it.Next(key));
                objects.Put(key, obj);
            }
        }

        ~ObjectLoader()
        {
            UnloadObjects();
        }

        size_t GetObjectCount() const
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            return objectCount;
        }

        // workaround for bug in gcc 4.0: fails to parse default function arguments in template classes
        bool LoadObjects()
        {
            return LoadObjects(false);
        }

        // load all dependencies of this type
        // return true if all are ready
        bool LoadObjects(bool wait)
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            if(objectCount == objects.GetSize())
            {
                // nothing to be done
                return true;
            }

            bool ready = true;
            typename HashType::GlobalIterator it(objects.GetIterator());
            while(it.HasNext())
            {
                HashObjectType& ref = it.Next();
                if(!ref.checked)
                {
                    ref.checked = ref.obj->Load(wait);
                    if(ref.checked)
                    {
                        ++objectCount;
                    }
                    else
                    {
                        ready = false;
                    }
                }
            }
            return ready;
        }

        // unloads all dependencies of this type
        void UnloadObjects()
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            if(!objectCount)
            {
                // nothing to be done
                return;
            }

            typename HashType::GlobalIterator it(objects.GetIterator());
            while(it.HasNext())
            {
                HashObjectType& ref = it.Next();
                if(ref.checked)
                {
                    ref.obj->Unload();
                    ref.checked = false;
                    --objectCount;
                }
            }
        }

        int UpdateObjects(const csBox3& loadBox, const csBox3& keepBox)
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            int oldObjectCount = objectCount;
            if(CS::Meta::IsBaseOf<RangeBased,T>::value)
            {
                typename HashType::GlobalIterator it(objects.GetIterator());
                while(it.HasNext())
                {
                    HashObjectType& ref = it.Next();
                    if(ref.checked)
                    {
                        if(ref.obj->OutOfRange(keepBox))
                        {
                            ref.obj->Unload();
                            ref.checked = false;
                            --objectCount;
                        }
                    }
                    else if(ref.obj->InRange(loadBox))
                    {
                        ref.checked = ref.obj->Load(false);
                        if(ref.checked)
                        {
                            ++objectCount;
                        }
                    }
                }
            }
            else
            {
                LoadObjects(false);
            }
            return (int)objectCount - oldObjectCount;
        }

        void AddDependency(T* obj)
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            if(!objects.Contains(obj->GetName()))
            {
                objects.Put(obj->GetName(), csRef<T>(obj));
            }
        }

        void RemoveDependency(const T* obj)
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            const HashObjectType& ref = objects.Get(obj->GetName(), HashObjectType(csRef<T>()));
            if(ref.checked)
            {
                ref.obj->Unload();
                --objectCount;
            }
            objects.DeleteAll(obj->GetName());
        }

        // workaround for bug in gcc 4.0: fails to parse default function argument in template classes
        const csRef<T>& GetDependency(const char* name) const
        {
            return GetDependency(name, csRef<T>());
        }

        const csRef<T>& GetDependency(const char* name, const csRef<T>& fallbackobj) const
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            const HashObjectType& ref = objects.Get(name,fallbackobj);
            return ref.obj;
        }

        bool HasDependency(const char* name) const
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            return objects.Contains(name);
        }

        HashType& GetDependencies()
        {
            return objects;
        }

        const HashType& GetDependencies() const
        {
            return objects;
        }

    protected:
        mutable CS::Threading::RecursiveMutex busy;

        HashType objects;
        size_t objectCount;
    };

    // actual world objects

    struct WaterArea
    {
        csBox3 bbox;
        csColor4 colour;
    };

    class ShaderVar : public CS::Utility::AtomicRefCount
    {
    public:
        ShaderVar(BgLoader* parent, ObjectLoader<Texture>* object) : parent(parent), object(object)
        {
        }

        ~ShaderVar()
        {
        }

        template<bool check> bool Parse(iDocumentNode* node, GlobalParserData& data);
        void LoadObject(csShaderVariable* var);
        CS::ShaderVarStringID GetID() const
        {
            return nameID;
        }

    private:
        CS::ShaderVarStringID nameID;
        csShaderVariable::VariableType type;
        csString value;
        float vec1;
        csVector2 vec2;
        csVector3 vec3;
        csVector4 vec4;

        BgLoader* parent;
        ObjectLoader<Texture>* object;
    };

    class Texture : public TrivialLoadable<iTextureWrapper,ObjectNames::texture>
    {
    public:
        Texture(BgLoader* parent) : TrivialLoadable<iTextureWrapper,ObjectNames::texture>(parent)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);
    };

    class Material : public Loadable, public ObjectLoader<Texture>
    {
    public:
        typedef iMaterialWrapper ObjectType;

        using ObjectLoader<Texture>::AddDependency;

        Material(BgLoader* parent) : Loadable(parent)
        {
        }

        bool Parse(iDocumentNode* node, GlobalParserData& data);

        bool LoadObject(bool wait);
        void UnloadObject();
        csPtr<iMaterialWrapper> GetObject()
        {
            csRef<iMaterialWrapper> wrapper(materialWrapper);
            return csPtr<iMaterialWrapper>(wrapper);
        }

    private:
        // dependencies
        struct Shader
        {
            csRef<iShader> shader;
            csStringID type;
        };
        csArray<Shader> shaders;
        csRefArray<ShaderVar> shadervars;

        // load data
        csRef<iMaterialWrapper> materialWrapper;
    };

    class MaterialLoader : public ObjectLoader<Material>
    {
    public:
        void ParseMaterialReference(GlobalParserData& data, const char* name, const char* parentName, const char* type);
    };

    class Trigger : public ObjectLoader<Sequence>,
                    public ObjectLoader<MeshObj>,
                    public ObjectLoader<Light>,
                    public TrivialLoadable<iSequenceTrigger,ObjectNames::trigger>,
                    public AlwaysLoaded
    {
    public:
        using ObjectLoader<Sequence>::AddDependency;
        using ObjectLoader<MeshObj>::AddDependency;
        using ObjectLoader<Light>::AddDependency;

        Trigger(BgLoader* parent) : TrivialLoadable<iSequenceTrigger,ObjectNames::trigger>(parent)
        {
        }

        bool LoadObject(bool wait)
        {
            bool ready = ObjectLoader<Sequence>::LoadObjects(true);
            ready &= ObjectLoader<MeshObj>::LoadObjects(true);
            ready &= ObjectLoader<Light>::LoadObjects(true);
            ready &= TrivialLoadable<iSequenceTrigger,ObjectNames::trigger>::LoadObject(true);
            return ready;
            //return TrivialLoadable<iSequenceTrigger,ObjectNames::trigger>::LoadObject(true);
        }

        void UnloadObject()
        {
            TrivialLoadable<iSequenceTrigger,ObjectNames::trigger>::UnloadObject();
            ObjectLoader<Sequence>::UnloadObjects();
            ObjectLoader<MeshObj>::UnloadObjects();
            ObjectLoader<Light>::UnloadObjects();
        }

        bool Parse(iDocumentNode* node, ParserData& data);
    };

    class Sequence : public ObjectLoader<Sequence>,
                     public ObjectLoader<Trigger>,
                     public ObjectLoader<Light>,
                     public ObjectLoader<MeshObj>,
                     public MaterialLoader,
                     public TrivialLoadable<iSequenceWrapper,ObjectNames::sequence>,
                     public AlwaysLoaded
    {
    public:
        using ObjectLoader<Sequence>::AddDependency;
        using ObjectLoader<Trigger>::AddDependency;
        using ObjectLoader<Light>::AddDependency;
        using ObjectLoader<MeshObj>::AddDependency;

        Sequence(BgLoader* parent) : TrivialLoadable<iSequenceWrapper,ObjectNames::sequence>(parent)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);

        bool LoadObject(bool wait)
        {
            // work around a race condition with engseqmgr
            wait = true;

            bool ready = true;
            if(ready)
            {
                ready = ObjectLoader<MeshObj>::LoadObjects(wait);
            }

            if(ready)
            {
                ready = ObjectLoader<Sequence>::LoadObjects(wait);
            }

            if(ready)
            {
                ready = TrivialLoadable<iSequenceWrapper,ObjectNames::sequence>::LoadObject(wait);
            }

            if(ready)
            {
                ready = ObjectLoader<Trigger>::LoadObjects(wait);
            }
            return ready;
        }

        void UnloadObject()
        {
            TrivialLoadable<iSequenceWrapper,ObjectNames::sequence>::UnloadObject();
            ObjectLoader<Sequence>::UnloadObjects();
            ObjectLoader<Trigger>::UnloadObjects();
            ObjectLoader<MeshObj>::UnloadObjects();
        }
    };

    class Light : public Loadable, public RangeBased
    {
    public:
        typedef iLight ObjectType;

        Light(BgLoader* parent) : Loadable(parent)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);

        bool LoadObject(bool wait);
        void UnloadObject();

        csPtr<iLight> GetObject()
        {
            return csPtr<iLight>(light);
        }

    private:
        // parse results
        csVector3 pos;
        float radius;
        csColor colour;
        csLightDynamicType dynamic;
        csLightAttenuationMode attenuation;
        csLightType type;

        // dependencies
        Sector* sector;

        // load data
        csRef<iLight> light;
    };

    class MeshFact : public TrivialLoadable<iMeshFactoryWrapper,ObjectNames::meshfact>,
                     public MaterialLoader
    {
    public:
        using ObjectLoader<Material>::AddDependency;

        MeshFact(BgLoader* parent) : TrivialLoadable<iMeshFactoryWrapper,ObjectNames::meshfact>(parent),
                                     cloned(false)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);
        bool ParseCells(iDocumentNode* node, ParserData& data);

        csPtr<MeshFact> Clone(const char* name)
        {
            csRef<MeshFact> clone;
            clone.AttachNew(new MeshFact(GetParent()));
            clone->SetName(name);

            clone->cloned = true;
            clone->parentFactory = &*this;
            return csPtr<MeshFact>(clone);
        }

        bool operator==(const MeshFact& other)
        {
            if(cloned != other.cloned)
            {
                if(parentFactory.IsValid())
                {
                    if(*parentFactory != other)
                    {
                        return false;
                    }
                }
                else
                {
                    if(*this != *other.parentFactory)
                    {
                        return false;
                    }
                }
            }
            else if(cloned)
            {
                if(parentFactory != other.parentFactory)
                    return false;
            }
            else
            {
                if(filename != other.filename)
                    return false;
                if((iDocumentNode*)data != (iDocumentNode*)other.data)
                    return false;
            }

            // ignore transformations for this check

            return true;
        }

        bool operator!=(const MeshFact& other)
        {
            return !(*this == other);
        }

        csPtr<iMeshFactoryWrapper> GetObject()
        {
            csRef<iMeshFactoryWrapper> object;
            if(cloned)
            {
                if(factory.IsValid())
                {
                    object = factory;
                }
                else if(parentFactory.IsValid())
                {
                    object = parentFactory->GetObject();
                }
            }
            else
            {
                object = TrivialLoadable<iMeshFactoryWrapper,ObjectNames::meshfact>::GetObject();
            }
            return csPtr<iMeshFactoryWrapper>(object);
        }

        void SetTransform(const csReversibleTransform& transform)
        {
            trans = transform;
        }

        bool LoadObject(bool wait);
        void UnloadObject();
        void FinishObject();

        bool FindSubmesh(const csString& name) const
        {
            return submeshes.Find(name) != csArrayItemNotFound;
        }

        const csArray<csVector3>& GetVertices() const
        {
            return bboxvs;
        }

    private:
        // cloning data
        bool cloned;
        csRef<MeshFact> parentFactory;
        csRef<iMeshFactoryWrapper> factory;

        // transformation data
        csReversibleTransform trans;

        // parser results
        csString filename;
        csArray<csVector3> bboxvs;
        csStringArray submeshes;
    };

    class MeshObj : public TrivialLoadable<iMeshWrapper,ObjectNames::meshobj>,
                    public RangeBased, public ObjectLoader<Texture>,
                    public MaterialLoader, public ObjectLoader<MeshFact>
    {
    public:
        using ObjectLoader<Texture>::AddDependency;
        using ObjectLoader<Material>::AddDependency;
        using ObjectLoader<MeshFact>::AddDependency;

        MeshObj(BgLoader* parent) : TrivialLoadable<iMeshWrapper,ObjectNames::meshobj>(parent)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data, bool& alwaysLoaded);
        bool ParseTriMesh(iDocumentNode* node, ParserData& data, bool& alwaysLoaded);

        bool LoadObject(bool wait);
        void UnloadObject();
        void FinishObject();

        bool FindSubmesh(const csString& name) const
        {
            typedef ObjectLoader<MeshFact>::HashType HashType;
            const HashType& factories = ObjectLoader<MeshFact>::GetDependencies();
            HashType::ConstGlobalIterator it(factories.GetIterator());
            bool found = false;
            while(it.HasNext() && !found)
            {
                const csRef<MeshFact>& factory = it.Next().obj;
                found |= factory->FindSubmesh(name);
            }
            return found;
        }

    private:
        // parse results
        bool dynamicLighting;

        // dependencies
        Sector* sector;
        csRefArray<ShaderVar> shadervars;
    };

    class MeshGen : public TrivialLoadable<iMeshGenerator,ObjectNames::meshgen>, public RangeBased, public MaterialLoader,
                    public ObjectLoader<MeshFact>
    {
    public:
        using ObjectLoader<MeshFact>::AddDependency;
        using ObjectLoader<Material>::AddDependency;

        MeshGen(BgLoader* parent) : TrivialLoadable<iMeshGenerator,ObjectNames::meshgen>(parent),
                                    sector(0)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);

        bool LoadObject(bool wait);
        void UnloadObject();

    private:
        // parser data
        Sector* sector;

        // dependencies
        csRef<MeshObj> mesh;
    };

    class Portal : public Loadable, public RangeBased
    {
    friend class Sector;
    public:
        typedef iMeshWrapper ObjectType;

        Portal(BgLoader* parent, float renderDist) : Loadable(parent), flags(0), renderDist(renderDist),
                                                     warp(false), ww_given(false), wv(0.f), ww(0.f),
                                                     autoresolve(0), targetSector(0), sector(0)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);

        bool LoadObject(bool wait);
        void UnloadObject();

        csPtr<iMeshWrapper> GetObject()
        {
            csRef<iMeshWrapper> obj(mObject);
            return csPtr<iMeshWrapper>(obj);
        }

    private:
        // parser results
        uint32 flags;
        float renderDist;
        
        // transformation data
        bool warp;
        bool ww_given;
        csMatrix3 matrix;
        csVector3 wv;
        csVector3 ww;
        csReversibleTransform transform;

        // sector data
        bool autoresolve;
        Sector* targetSector;
        Sector* sector;

        // vertex data
        csPoly3D poly;

        // load data
        iPortal* pObject;
        csRef<iMeshWrapper> mObject;

        // sector callback
        class MissingSectorCallback : public scfImplementation1<MissingSectorCallback, iPortalCallback>
        {
        private:
            csRef<Sector> targetSector;
            bool autoresolve;

        public:
            MissingSectorCallback(Sector* target, bool resolve) : scfImplementationType(this),targetSector(target),autoresolve(resolve)
            {
            }

            virtual ~MissingSectorCallback()
            {
            }

            virtual bool Traverse(iPortal* p, iBase* /*context*/)
            {
                csRef<iSector> target = targetSector->GetObject();
                if(target.IsValid())
                {
                    p->SetSector(target);
                }
                else
                {
                    return false;
                }

                if(!autoresolve)
                {
                    p->RemoveMissingSectorCallback(this);
                }

                return true;
            }
        };
    };

    class Sector : public Loadable,
                   public ObjectLoader<MeshGen>, public ObjectLoader<MeshObj>,
                   public ObjectLoader<Portal>, public ObjectLoader<Light>,
                   public ObjectLoader<Sequence>, public ObjectLoader<Trigger>
    {
    public:
        using ObjectLoader<MeshGen>::AddDependency;
        using ObjectLoader<MeshObj>::AddDependency;
        using ObjectLoader<Portal>::AddDependency;
        using ObjectLoader<Light>::AddDependency;
        using ObjectLoader<Sequence>::AddDependency;
        using ObjectLoader<Trigger>::AddDependency;

        Sector(BgLoader* parent) : Loadable(parent), ambient(0.0f), objectCount(0),
                                   init(false), isLoading(false)
        {
        }

        ~Sector()
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);

        bool LoadObject(bool wait);
        void UnloadObject();
        int UpdateObjects(const csBox3& loadBox, const csBox3& keepBox, size_t recursions);

        bool Initialize();
        void ForceUpdateObjectCount();
        void FindConnectedSectors(csSet<csPtrKey<Sector> >& connectedSectors);

        void AddPortal(Portal* p)
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            activePortals.Add(p);
        }

        void RemovePortal(Portal* p)
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            activePortals.Delete(p);
        }

        void AddAlwaysLoaded(MeshObj* mesh)
        {
            alwaysLoaded.AddDependency(mesh);
        }

        csPtr<iSector> GetObject()
        {
            csRef<iSector> obj(object);
            return csPtr<iSector>(obj);
        }

    private:
        // parser results
        csString culler;
        csColor ambient;
        size_t objectCount;
        Zone* parent;

        // dependencies
        csArray<WaterArea> waterareas;
        ObjectLoader<MeshObj> alwaysLoaded;

        // load data
        CS::Threading::RecursiveMutex busy;
        csRef<iSector> object;
        csSet<csPtrKey<Portal> > activePortals;
        bool init;
        bool isLoading;
    };

    // Stores world representation.
    class Zone : public Loadable, public ObjectLoader<Sector>
    {
    public:
        using ObjectLoader<Sector>::AddDependency;

        Zone(BgLoader* parent, const char* name)
            : Loadable(parent), priority(false)
        {
            Loadable::SetName(name);
        }

        bool LoadObject(bool wait)
        {
            return ObjectLoader<Sector>::LoadObjects(wait);
        }

        void UnloadObject()
        {
            ObjectLoader<Sector>::UnloadObjects();
        }

        void UpdatePriority(bool newPriority)
        {
            if(priority != newPriority)
            {
                priority = newPriority;

                // upgrade sector priorities
                /*ObjectLoader<Sector>::HashType::GlobalIterator it(ObjectLoader<Sector>::objects.GetIterator());
                while(it.HasNext())
                {
                    it.Next().obj->priority = newPriority;
                }*/
            }
        }
        
        bool GetPriority() const
        {
            return priority;
        }

    private:
        // loader data
        bool priority;
    };

    struct GlobalParserData
    {
        // token lookup table
        csStringHash xmltokens;

        // temporary parse-time data
        LockedType<Texture> textures;
        LockedType<MeshObj> meshes;
        csRef<iShaderVarStringSet> svstrings;

        // plugin references
        csRef<iSyntaxService> syntaxService;
        iObjectRegistry* object_reg;

        // persistent data
        csRef<iStringSet> strings;
        LockedType<Material> materials;
        LockedType<Sector> sectors;
        LockedType<MeshFact> factories;
        LockedType<Zone> zones;
        LockedType<StartPosition,false> positions;
        CS::Threading::ReadWriteMutex shaderLock;
        csHash<csString, csStringID> shadersByUsage;
        csStringArray shaders;
        csHash<csString, csString> shaderAliases;

        // config
        struct ParserConfig
        {
            bool cache;
            uint enabledGfxFeatures;
            bool portalsOnly;
            bool meshesOnly;
            bool parseShaders;
            bool blockShaderLoad;
            bool parsedShaders;
            bool parseShaderVars;
            bool forceCuller;
            csString culler;
        } config;
    } parserData;

    struct ParserData
    {
        ParserData(GlobalParserData& data) : data(data)
        {
        }

        // global data
        GlobalParserData& data;

        // temporary data used on a per-library basis
        csRefArray<iThreadReturn> rets;
        Zone* zone;
        csRef<Sector> currentSector;
        LockedType<Light> lights;
        LockedType<Sequence> sequences;
        LockedType<Trigger> triggers;
        csString vfsPath;
        csString path;
        bool realRoot;
        bool parsedMeshFact;
    };

    /***********************************************************************/

    /* Parsing Methods */
    // parser tokens
#define CS_TOKEN_ITEM_FILE "src/plugins/common/bgloader/parser.tok"
#define CS_TOKEN_LIST_TOKEN_PREFIX PARSERTOKEN_
#include "cstool/tokenlist.h"
#undef CS_TOKEN_ITEM_FILE
#undef CS_TOKEN_LIST_TOKEN_PREFIX

    void ParseMaterials(iDocumentNode* materialsNode);

    /* shader methods */
    void ParseShaders();

    /* Internal unloading methods. */
    void CleanDisconnectedSectors(Sector* sector);

    bool LoadSequencesAndTriggers (iDocumentNode* snode, iDocumentNode* tnode, ParserData& data);

    // Pointers to other needed plugins.
    iObjectRegistry* object_reg;
    csRef<iEngine> engine;
    csRef<iGraphics2D> g2d;
    csRef<iThreadedLoader> tloader;
    csRef<iThreadManager> tman;
    csRef<iVFS> vfs;
    csRef<iCollideSystem> cdsys;
    CS::Threading::RecursiveMutex vfsLock;

    // currently loaded zones - used by zone-based loading
    ObjectLoader<Zone> loadedZones;

    // currently loading objects
    CS::Threading::RecursiveMutex loadLock;
    size_t loadOffset;
    size_t delayedOffset;
    csArray<csPtrKey<Loadable> > loadList;
    csArray<csPtrKey<iDelayedLoader> > delayedLoadList;

    // Our load range ^_^
    float loadRange;

    // Whether the current position is valid.
    bool validPosition;

    // Limit on how many portals deep we load.
    uint maxPortalDepth;

    // Number of checks an object may be lingering
    // without aborting the load
    size_t maxLingerCount;

    // The last valid sector.
    csRef<Sector> lastSector;

    // The last valid position.
    csVector3 lastPos;

    // current load step - use for ContinueLoading
    size_t loadStep;

    // For world manipulation.
    csRef<iMeshWrapper> selectedMesh;
    csRef<MeshFact> selectedFactory;
    csRef<Material> selectedMaterial;
    csVector2 previousPosition;
    csVector3 origTrans;
    csVector3 rotBase;
    csVector3 origRot;
    csVector3 currRot_h;
    csVector3 currRot_v;
    bool resetHitbeam;
};
}
CS_PLUGIN_NAMESPACE_END(bgLoader)

#endif // __LOADER_H__

