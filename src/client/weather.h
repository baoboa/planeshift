
#ifndef MODE_WEATHER_HEADER
#define MODE_WEATHER_HEADER


#include "isndsys/ss_structs.h"
#include "isndsys/ss_data.h"
#include "isndsys/ss_stream.h"
#include "isndsys/ss_source.h"

/* Defines for the various weather conditions. Some things may need to trigger on 
   a  'general' time of day. */
enum WeatherConditions
{
    WEATHER_ANY,
    WEATHER_CLEAR,
    WEATHER_RAIN,
    WEATHER_SNOW,
    WEATHER_FOG
};

enum WeatherSound
{
    WEATHER_SOUND_CLEAR = 1,
    WEATHER_SOUND_RAIN_LIGHT,
    WEATHER_SOUND_RAIN_MEDIUM,
    WEATHER_SOUND_RAIN_HEAVY,
    WEATHER_SOUND_SNOW_LIGHT,
    WEATHER_SOUND_SNOW_HEAVY,
    WEATHER_SOUND_FOG
};

class WeatherObject;

struct WeatherParams
{
    int value;
    int fade_value;
    int fade_time;
};


/// Struct for information about current weather
struct WeatherInfo
{
    csString sector;
    WeatherConditions downfall_condition;
    WeatherParams     downfall_params;
    WeatherConditions fog_condition;
    WeatherParams     fog_params;

    WeatherObject*    fog;
    
    int r,g,b;

    static void Fade(WeatherParams* wp, int delta);
};

//-----------------------------------------------------------------------------

/** Holds the weather object, need this because some things have been created
  *  but do not have an object persay
  */
class WeatherObject
{
public:
    WeatherObject(WeatherInfo* parent);
    virtual ~WeatherObject();

    // General
    /** Destory this weather object and remove any required meshes. */
    virtual void Destroy();
        
    virtual bool Valid();
    virtual void MoveTo(WeatherInfo* new_parent,iSector* sector);
    virtual void MoveTo(csVector3 pos);
    virtual void StartFollow();
    virtual void StopFollow();
    virtual iSector* GetSector();
    // If you use this function, you need to call SetupMesh and stuff like that too
    virtual void SetParent(WeatherInfo* new_parent) { parent = new_parent;}
    virtual WeatherInfo* GetParent() {return parent;}
    virtual void SetColor(float /*r*/, float /*g*/, float /*b*/){};
    virtual void Update(csTicks /*delta*/){};
    
    // Specific
    virtual bool CreateMesh() = 0;
    virtual void SetupMesh(csBox3 bbox) = 0;
    virtual WeatherSound GetWeatherSound()      { return WEATHER_SOUND_CLEAR; }
    virtual WeatherSound GetWeatherSoundForced(){ return WEATHER_SOUND_CLEAR; }

    virtual WeatherConditions GetType() { return WEATHER_ANY; }

    // Calculations
    //virtual float GetDensity(int drops) = 0;
    virtual csBox3 CreateDefaultBBox() = 0;

protected:

    void RefreshSector(); // For flags

    // standard members
    csBox3                      bbox; // Used for sector movements
    WeatherInfo*                parent;
    csRef<iMeshWrapper>         mesh;
    csRef<iMeshFactoryWrapper>  mfw;
    csRef<iMaterialWrapper>     mat;

    //csRef<iSndSysSource>        weather_sound;  // looping background weather sound
};

class RainWeatherObject;
class SnowWeatherObject;
class FogWeatherObject;

//-----------------------------------------------------------------------------


/// Rain, also handles a fog object
class RainWeatherObject : public WeatherObject
{
public:
    RainWeatherObject(WeatherInfo* parent);
    virtual ~RainWeatherObject();

    void Destroy();
    void MoveTo(WeatherInfo* newParent,iSector* sector);

    bool CreateMesh();
    void SetupMesh(csBox3 bbox);
    void SetDrops(int drops);

    virtual void Update(csTicks delta);
    
    WeatherSound      GetWeatherSound();
    WeatherSound      GetWeatherSoundForced();
    WeatherConditions GetType() { return WEATHER_RAIN; }

    // Calculations
    static float GetDensity(int drops);
    csBox3 CreateDefaultBBox();
};

//-----------------------------------------------------------------------------

/// Snow
class SnowWeatherObject : public WeatherObject
{
public:
    SnowWeatherObject(WeatherInfo* parent);
    virtual ~SnowWeatherObject();

    void Destroy();
    void MoveTo(WeatherInfo* newParent,iSector* sector);

    bool CreateMesh();
    void SetupMesh(csBox3 bbox);
    void SetDrops(int drops);

    virtual void Update(csTicks delta);

    WeatherSound      GetWeatherSound();
    WeatherSound      GetWeatherSoundForced();
    WeatherConditions GetType() { return WEATHER_SNOW; }

    // Calculations
    static float GetDensity(int drops);
    csBox3 CreateDefaultBBox();

private:
    csRef<csShaderVariable> snowDensitySV;
    CS::ShaderVarStringID snowDensity;
};

//-----------------------------------------------------------------------------

/// Fog
class FogWeatherObject : public WeatherObject
{
public:
    FogWeatherObject(WeatherInfo* parent);
    virtual ~FogWeatherObject();

    void Destroy();
    bool Valid();
    bool CreateMesh();
    void MoveTo(WeatherInfo* newParent,iSector* sector);
    void SetupMesh(csBox3 /*bbox*/){}
    iSector* GetSector();

    void MoveTo(csVector3 /*pos*/)  {}
    void StartFollow()          {}
    void StopFollow()           {}

    void SetColor(float r,float g,float b);
    virtual void Update(csTicks delta);

    WeatherSound      GetWeatherSound();
    WeatherSound      GetWeatherSoundForced();
    WeatherConditions GetType() { return WEATHER_FOG; }

    // Calculations
    static float GetDensity(int density);
    csBox3 CreateDefaultBBox()  {return csBox3(0,0,0,0,0,0); }

    WeatherInfo* GetWeatherInfo() { return parent;} // WeatherInfos are noremaly created for these objects
private:
    bool applied;

    csColor color;
    iSector* sector;
};

#endif
