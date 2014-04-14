/*
    Crystal Space Entity Layer
    Copyright (C) 2010 by Leonardo Rodrigo Domingues
    Copyright (C) 2011 by Matthieu Kraus
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#include "celnavmesh.h"
#include <csgeom/math3d.h>

CS_PLUGIN_NAMESPACE_BEGIN(celNavMesh)
{

inline unsigned int ilog2 (unsigned int v)
{
  unsigned int r;
  unsigned int shift;
  r = (v > 0xffff) << 4; v >>= r;
  shift = (v > 0xff) << 3; v >>= shift; r |= shift;
  shift = (v > 0xf) << 2; v >>= shift; r |= shift;
  shift = (v > 0x3) << 1; v >>= shift; r |= shift;
  r |= (v >> 1);
  return r;
}

/*
 * DebugDrawCS
 */

DebugDrawCS::DebugDrawCS ()
{
  currentMesh = 0;
  currentZBufMode = CS_ZBUF_USE;
  nVertices = 0;
  meshes = new csArray<csSimpleRenderMesh*>();
}

DebugDrawCS::~DebugDrawCS ()
{
  delete currentMesh;
}

void DebugDrawCS::depthMask (bool state)
{
  if (state)
  {
    currentZBufMode = CS_ZBUF_USE;
  }
  else
  {
    currentZBufMode = CS_ZBUF_TEST;
  }
}

void DebugDrawCS::texture(bool state)
{

}

void DebugDrawCS::begin (duDebugDrawPrimitives prim, float size)
{  
  currentMesh = new csSimpleRenderMesh();
  //meshes->DeleteAll();
  currentMesh->z_buf_mode = currentZBufMode;
  currentMesh->alphaType.autoAlphaMode = false;
  currentMesh->alphaType.alphaType = currentMesh->alphaType.alphaSmooth;
  switch (prim)
  {
  case DU_DRAW_POINTS:
    currentMesh->meshtype = CS_MESHTYPE_POINTS;
    break;
  case DU_DRAW_LINES:
    currentMesh->meshtype = CS_MESHTYPE_LINES;
    break;
  case DU_DRAW_TRIS:
    currentMesh->meshtype = CS_MESHTYPE_TRIANGLES;
    break;
  case DU_DRAW_QUADS:
    currentMesh->meshtype = CS_MESHTYPE_QUADS;
    break;
  };
}

void DebugDrawCS::vertex (const float* pos, unsigned int color)
{
  vertex(pos[0], pos[1], pos[2], color);
}

void DebugDrawCS::vertex (const float x, const float y, const float z, unsigned int color)
{
  float r = (color & 0xFF) / 255.0f;
  float g = ((color >> 8) & 0xFF) / 255.0f;
  float b = ((color >> 16) & 0xFF) / 255.0f;
  float a = ((color >> 24) & 0xFF) / 255.0f;
  vertices.Push(csVector3(x, y, z));
  colors.Push(csVector4(r, g, b, a));
  nVertices++;
}

void DebugDrawCS::vertex (const float* pos, unsigned int color, const float* uv)
{
  vertex(pos[0], pos[1], pos[2], color);
}

void DebugDrawCS::vertex (const float x, const float y, const float z, unsigned int color, const float u, const float v)
{
  float r = (color & 0xFF) / 255.0f;
  float g = ((color >> 8) & 0xFF) / 255.0f;
  float b = ((color >> 16) & 0xFF) / 255.0f;
  float a = ((color >> 24) & 0xFF) / 255.0f;
  vertices.Push(csVector3(x, y, z));
  colors.Push(csVector4(r, g, b, a));
  nVertices++;
}

void DebugDrawCS::end ()
{
  csVector3* verts = new csVector3[nVertices];
  csVector4* cols = new csVector4[nVertices];
  csArray<csVector3>::Iterator vertsIt = vertices.GetIterator();
  csArray<csVector4>::Iterator colsIt = colors.GetIterator();
  int index = 0;
  while (vertsIt.HasNext())
  {
    verts[index] = vertsIt.Next();
    cols[index] = colsIt.Next();
    index++;
  }

  currentMesh->vertices = verts;
  currentMesh->colors = cols;
  currentMesh->vertexCount = nVertices;
  meshes->Push(currentMesh); 

  nVertices = 0;
  vertices.DeleteAll();
  colors.DeleteAll();  
  //delete currentMesh;
  currentMesh = 0;
}

csArray<csSimpleRenderMesh*>* DebugDrawCS::GetMeshes ()
{
  return meshes;
}



/*
 * celNavMeshPath
 */

const int celNavMeshPath::INCREASE_PATH_BY = 10;

celNavMeshPath::celNavMeshPath (float* path, int pathSize, int maxPathSize, iSector* sector) 
    : scfImplementationType (this)
{
  this->path = path;
  this->pathSize = pathSize;
  this->maxPathSize = maxPathSize;
  currentPosition = 0;
  increasePosition = 3;
  this->sector = sector;
  debugMeshes = new csArray<csSimpleRenderMesh*>();
}

celNavMeshPath::~celNavMeshPath ()
{
  delete [] path;
  if (!debugMeshes->IsEmpty()) 
  { 
    csArray<csSimpleRenderMesh*>::Iterator it = debugMeshes->GetIterator(); 
    while (it.HasNext()) 
    { 
      csSimpleRenderMesh* mesh = it.Next(); 
      delete [] mesh->vertices; 
      delete [] mesh->colors;  
    } 
    debugMeshes->DeleteAll();
  }
  delete debugMeshes;
}

iSector* celNavMeshPath::GetSector () const
{
  return sector;
}

void celNavMeshPath::Current (csVector3& vector) const
{
  vector[0] = path[currentPosition];
  vector[1] = path[currentPosition + 1];
  vector[2] = path[currentPosition + 2];
}

void celNavMeshPath::Next (csVector3& vector)
{
  currentPosition += increasePosition;
  vector[0] = path[currentPosition];
  vector[1] = path[currentPosition + 1];
  vector[2] = path[currentPosition + 2];
}

void celNavMeshPath::Previous (csVector3& vector)
{
  currentPosition -= increasePosition;
  vector[0] = path[currentPosition];
  vector[1] = path[currentPosition + 1];
  vector[2] = path[currentPosition + 2];
}

void celNavMeshPath::GetFirst (csVector3& vector) const
{
  int index = (increasePosition > 0) ? 0 : ((pathSize - 1) * 3);
  vector[0] = path[index];
  vector[1] = path[index + 1];
  vector[2] = path[index + 2];
}

void celNavMeshPath::GetLast (csVector3& vector) const
{
  int index = (increasePosition > 0) ? ((pathSize - 1) * 3) : 0;
  vector[0] = path[index];
  vector[1] = path[index + 1];
  vector[2] = path[index + 2];
}

bool celNavMeshPath::HasNext () const
{
  if (increasePosition > 0)
  {
    if (currentPosition < (pathSize - 1) * 3)
    {
      return true;
    }
  }
  else
  {
    if (currentPosition >= 3)
    {
      return true;
    }
  }
  return false;
}

bool celNavMeshPath::HasPrevious () const
{
  if (increasePosition > 0)
  {
    if (currentPosition >= 3)
    {
      return true;
    }
  }
  else
  {
    if (currentPosition < (pathSize - 1) * 3)
    {
      return true;
    }
  }
  return false;
}

void celNavMeshPath::Invert ()
{
  increasePosition = -increasePosition;
}

void celNavMeshPath::Restart ()
{
  if (increasePosition > 0)
  {
    currentPosition = 0;
  }
  else
  {
    currentPosition = (pathSize - 1) * 3;
  }
}

void celNavMeshPath::AddNode (csVector3 node) 
{
  if (pathSize == maxPathSize)
  {
    float* newPath = new float[(maxPathSize + INCREASE_PATH_BY) * 3];
    memcpy(newPath, path, pathSize * 3 * sizeof(float));
    delete [] path;
    path = newPath;
    maxPathSize += INCREASE_PATH_BY;
  }
  int index = pathSize * 3;
  path[index] = node[0];
  path[index + 1] = node[1];
  path[index + 2] = node[2];
  pathSize++;
}

void celNavMeshPath::InsertNode (int pos, csVector3 node)
{
  int index = pos * 3;
  if (pathSize == maxPathSize)
  {
    float* newPath = new float[(maxPathSize + INCREASE_PATH_BY) * 3];
    memcpy(newPath, path, (pos * 3) * sizeof(float));
    memcpy(newPath + ((pos + 1) * 3), path + (pos * 3), (pathSize - pos) * 3 * sizeof(float));
    delete [] path;
    path = newPath;
    maxPathSize += INCREASE_PATH_BY;
  }
  else
  {
    memmove(path + ((pos + 1) * 3), path + (pos * 3), (pathSize - pos) * 3 * sizeof(float));
  }
  path[index] = node[0];
  path[index + 1] = node[1];
  path[index + 2] = node[2];
  pathSize++;
}

float celNavMeshPath::Length() const
{
  float length = 0.0f;
  int index;
  float f0, f1, f2;
  for (int i = 1; i < pathSize; i++)
  {
    index = i * 3;
    f0 = (path[index] - path[index - 3]);
    f1 = (path[index + 1] - path[index - 2]);
    f2 = (path[index + 2] - path[index - 1]);
    length += csQsqrt(f0 * f0 + f1 * f1 + f2 * f2);
  }

  return length;
}

int celNavMeshPath::GetNodeCount () const
{
  return pathSize;
}

// Based on Detour NavMeshTesterTool::handleRender()
csArray<csSimpleRenderMesh*>* celNavMeshPath::GetDebugMeshes ()
{
  if (pathSize)
  {
    DebugDrawCS dd;
    dd.depthMask(false);
    const unsigned int pathCol = duRGBA(255, 255, 255, 230);
    dd.begin(DU_DRAW_LINES, 4.0f);
    for (int i = 0; i < pathSize - 1; ++i)
    {
      unsigned int col = pathCol;
      dd.vertex(path[i * 3], path[i * 3 + 1] + 0.4f, path[i * 3 + 2], col);
      dd.vertex(path[(i + 1) * 3], path[(i + 1) * 3 + 1] + 0.4f, path[(i + 1) * 3 + 2], col);
    }
    dd.end();
    dd.begin(DU_DRAW_POINTS, 10.0f);
    for (int i = 0; i < pathSize; ++i)
    {
      dd.vertex(path[i*3], path[i * 3 + 1] + 0.4f, path[i * 3 + 2], duRGBA(255, 150, 0, 230));
    }
    dd.end();
    dd.depthMask(true);

    // Clear previous meshes
    if (!debugMeshes->IsEmpty()) 
    { 
      csArray<csSimpleRenderMesh*>::Iterator it = debugMeshes->GetIterator(); 
      while (it.HasNext()) 
      { 
        csSimpleRenderMesh* mesh = it.Next(); 
        delete [] mesh->vertices; 
        delete [] mesh->colors; 
      }
      debugMeshes->DeleteAll(); 
    }

    // Update meshes
    debugMeshes = dd.GetMeshes();
    return debugMeshes;
  }
  return 0;
}



/*
 * celNavMesh
 */

const int celNavMesh::MAX_NODES = 2048;
const int celNavMesh::NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
const int celNavMesh::NAVMESHSET_VERSION = 1;

celNavMesh::celNavMesh (iObjectRegistry* objectRegistry) : scfImplementationType (this)
{
  parameters = 0;
  sector = 0;
  detourNavMesh = 0;
  detourNavMeshQuery = 0;
  navMeshDrawFlags = DU_DRAWNAVMESH_OFFMESHCONS; 
  filter.setIncludeFlags(SAMPLE_POLYFLAGS_ALL);
  filter.setExcludeFlags(0);
  this->objectRegistry = objectRegistry;
  debugMeshes = new csArray<csSimpleRenderMesh*>();
  agentDebugMeshes = new csArray<csSimpleRenderMesh*>();
}

celNavMesh::~celNavMesh ()
{  
  if (!agentDebugMeshes->IsEmpty()) 
  { 
    csArray<csSimpleRenderMesh*>::Iterator it = agentDebugMeshes->GetIterator(); 
    while (it.HasNext()) 
    { 
      csSimpleRenderMesh* mesh = it.Next(); 
      delete [] mesh->vertices; 
      delete [] mesh->colors; 
    }
    agentDebugMeshes->DeleteAll(); 
  }
  delete agentDebugMeshes;

  if (!debugMeshes->IsEmpty()) 
  { 
    csArray<csSimpleRenderMesh*>::Iterator it = debugMeshes->GetIterator(); 
    while (it.HasNext()) 
    { 
      csSimpleRenderMesh* mesh = it.Next(); 
      delete [] mesh->vertices; 
      delete [] mesh->colors; 
    }
    debugMeshes->DeleteAll(); 
  }
  delete debugMeshes;

  delete detourNavMesh;
  delete detourNavMeshQuery;
}

 // Based on Recast Sample_TileMesh::handleBuild() and Sample_TileMesh::handleSettings()
bool celNavMesh::Initialize (const iCelNavMeshParams* parameters, iSector* sector, 
                             const float* boundingMin, const float* boundingMax)
{
  this->parameters.AttachNew(new celNavMeshParams(parameters));
  this->sector = sector;
  rcVcopy(this->boundingMin, boundingMin);
  rcVcopy(this->boundingMax, boundingMax);

  int gw = 0, gh = 0;
  rcCalcGridSize(boundingMin, boundingMax, parameters->GetCellSize(), &gw, &gh);
  const int ts = parameters->GetTileSize();
  const int tw = (gw + ts - 1) / ts;
  const int th = (gh + ts - 1) / ts;
  
  // Max tiles and max polys affect how the tile IDs are caculated.
  // There are 22 bits available for identifying a tile and a polygon.
  int tileBits = rcMin((int)ilog2(csFindNearestPowerOf2 (tw * th)), 14);
  if (tileBits > 14)
  {
    tileBits = 14;
  }
  int polyBits = 22 - tileBits;
  int maxTiles = 1 << tileBits;
  int maxPolysPerTile = 1 << polyBits;

  dtNavMeshParams params;
  params.orig[0] = boundingMin[0];
  params.orig[1] = boundingMin[1];
  params.orig[2] = boundingMin[2];
  params.tileWidth = parameters->GetTileSize() * parameters->GetCellSize();
  params.tileHeight = params.tileWidth;
  params.maxTiles = maxTiles;
  params.maxPolys = maxPolysPerTile;
  detourNavMesh = new dtNavMesh;
  detourNavMeshQuery = new dtNavMeshQuery();

  if (detourNavMesh->init(&params)== DT_SUCCESS)
  {
    if (detourNavMeshQuery->init(detourNavMesh, MAX_NODES) == DT_SUCCESS)
    {
      return true;
    }
  }
  
  return false;
}

csArray<csPoly3D> celNavMesh::QueryPolygons(const csBox3& box) const
{
  csArray<csPoly3D> result;

  // create a detour bbox
  float center[3];
  float extent[3];
  box.GetCenter().Get(center);
  (box.Max()-box.GetCenter()).Get(extent);

  // get polygon references
  dtPolyRef polyRefs[128];
  int polyCount;
  detourNavMeshQuery->queryPolygons(center,extent,&filter,polyRefs,&polyCount,128);

  // process the references
  for(int i = 0; i < polyCount; i++)
  {
    // find the tile this poly belongs to and the poly itself
    const dtMeshTile* tile;  
    const dtPoly* detourPoly; 
    detourNavMesh->getTileAndPolyByRef(polyRefs[i], &tile, &detourPoly);

    // convert detour poly to cs poly
    csPoly3D poly(detourPoly->vertCount);
    for(int j = 0; j < detourPoly->vertCount; ++j)
    {
      poly.AddVertex(tile->verts[detourPoly->verts[j]*3],
                     tile->verts[detourPoly->verts[j]*3+1],
                     tile->verts[detourPoly->verts[j]*3+2]);
    }

    // add it to the result array
    result.Push(poly);
  }

  return result;
}

// Based on Recast NavMeshTesterTool::recalc()
iCelNavMeshPath* celNavMesh::ShortestPath (const csVector3& from, const csVector3& goal, const int maxPathSize)
{
  float startPos[3];
  float endPos[3];
  for (int i = 0; i < 3; i++) 
  {
    startPos[i] = from[i];
    endPos[i] = goal[i];
  }

  // Find nearest polygons around the origin and destination of the path
  float polyPickExt[3];
  polyPickExt[0] = parameters->GetPolygonSearchBox()[0];
  polyPickExt[1] = parameters->GetPolygonSearchBox()[1];
  polyPickExt[2] = parameters->GetPolygonSearchBox()[2];
  dtPolyRef startRef; 
  detourNavMeshQuery->findNearestPoly(startPos, polyPickExt, &filter, &startRef, 0);
  dtPolyRef endRef;
  detourNavMeshQuery->findNearestPoly(endPos, polyPickExt, &filter, &endRef, 0);

  // Find the polygons that compose the path
  dtPolyRef* polys = new dtPolyRef[maxPathSize];
  int npolys = 0;
  detourNavMeshQuery->findPath(startRef, endRef, startPos, endPos, &filter, polys, &npolys, maxPathSize);

  // Find the actual path inside those polygons
  float* straightPath = new float[maxPathSize * 3];
  unsigned char* straightPathFlags = new unsigned char[maxPathSize];
  dtPolyRef* straightPathPolys = new dtPolyRef[maxPathSize];
  int nstraightPath = 0;
  if (npolys > 0)
  {
    detourNavMeshQuery->findStraightPath(startPos, endPos, polys, npolys, straightPath, 
                                         straightPathFlags, straightPathPolys, &nstraightPath, maxPathSize);
  }
  if (nstraightPath > 0)
  {
    path.AttachNew(new celNavMeshPath(straightPath, nstraightPath, maxPathSize, sector));
  }
  else
  {
    delete [] straightPath;
    path.AttachNew(new celNavMeshPath(0, 0, 0, 0));
  }

  // For now, these are not really used
  delete [] polys;
  delete [] straightPathFlags;
  delete [] straightPathPolys;

  return path;
}

bool celNavMesh::Update (const csBox3& boundingBox)
{
  // Construct a new builder interface
  csRef<celNavMeshBuilder> builder;
  builder.AttachNew(new celNavMeshBuilder(0));
  builder->Initialize(objectRegistry);
  builder->SetNavMeshParams(parameters);
  builder->SetSector(sector);

  return builder->UpdateNavMesh(this, boundingBox);
}

bool celNavMesh::Update (const csOBB& boundingBox)
{
  csVector3 min (0.0f);
  csVector3 max (0.0f);
  for (int i = 0; i < 8; i++)
  {
    csVector3 v = boundingBox.GetCorner(i);
    for (int j = 0; j < 3; j++)
    {
      if (v[j] < min[j])
      {
        min[j] = v[j];
      }
      if (v[j] > max[j])
      {
        max[j] = v[j];
      }
    }
  }

  csBox3 aabb(min, max);

  return Update(aabb);
}

bool celNavMesh::AddTile (unsigned char* data, int dataSize)
{
  dtTileRef result;
  detourNavMesh->addTile(data, dataSize, 0, 0, &result);
  if (result)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool celNavMesh::RemoveTile (int x, int y)
{
  if (detourNavMesh->removeTile(detourNavMesh->getTileRefAt(x, y, 0), 0, 0) == DT_SUCCESS)
  {
    return true;
  }
  else return false;
}

iSector* celNavMesh::GetSector () const
{
  return sector;
}

void celNavMesh::SetSector (iSector* sector)
{
  this->sector = sector;
}

iCelNavMeshParams* celNavMesh::GetParameters () const
{
  return parameters;
}

csBox3 celNavMesh::GetBoundingBox() const
{
  return csBox3(boundingMin[0], boundingMin[1], boundingMin[2], boundingMax[0], boundingMax[1], boundingMax[2]);
}

bool celNavMesh::SaveToFile (iFile* file) const
{
  csRef<iDocumentSystem> docsys = csLoadPluginCheck<iDocumentSystem>(objectRegistry, "crystalspace.documentsystem.tinyxml");
  if (!docsys)
  {
    return false;
  }
  
  // Create XML file
  csRef<iDocument> doc = docsys->CreateDocument();
  csRef<iDocumentNode> root = doc->CreateRoot();
  csRef<iDocumentNode> mainNode = root->CreateNodeBefore(CS_NODE_ELEMENT);
  mainNode->SetValue("iCelNavMesh");
  mainNode->SetAttribute("sector", sector->QueryObject()->GetName());
  
  // Bounding box node
  csRef<iDocumentNode> boundingBoxNode = mainNode->CreateNodeBefore(CS_NODE_ELEMENT);
  boundingBoxNode->SetValue("boundingbox");
  csRef<iDocumentNode> min = boundingBoxNode->CreateNodeBefore(CS_NODE_ELEMENT);
  min->SetValue("min");
  min->SetAttributeAsFloat("x", boundingMin[0]);
  min->SetAttributeAsFloat("y", boundingMin[1]);
  min->SetAttributeAsFloat("z", boundingMin[2]);
  csRef<iDocumentNode> max = boundingBoxNode->CreateNodeBefore(CS_NODE_ELEMENT);
  max->SetValue("max");
  max->SetAttributeAsFloat("x", boundingMax[0]);
  max->SetAttributeAsFloat("y", boundingMax[1]);
  max->SetAttributeAsFloat("z", boundingMax[2]);

  // Create parameters node
  csRef<iDocumentNode> parametersNode = mainNode->CreateNodeBefore(CS_NODE_ELEMENT);
  parametersNode->SetValue("parameters");
  csRef<iDocumentNode> node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("agentheight");
  node->SetAttributeAsFloat("value", parameters->GetAgentHeight());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("agentradius");
  node->SetAttributeAsFloat("value", parameters->GetAgentRadius());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("agentmaxslopeangle");
  node->SetAttributeAsFloat("value", parameters->GetAgentMaxSlopeAngle());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("agentmaxclimb");
  node->SetAttributeAsFloat("value", parameters->GetAgentMaxClimb());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("cellsize");
  node->SetAttributeAsFloat("value", parameters->GetCellSize());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("cellheight");
  node->SetAttributeAsFloat("value", parameters->GetCellHeight());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("maxsimplificationerror");
  node->SetAttributeAsFloat("value", parameters->GetMaxSimplificationError());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("detailsampledist");
  node->SetAttributeAsFloat("value", parameters->GetDetailSampleDist());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("detailsamplemaxerror");
  node->SetAttributeAsFloat("value", parameters->GetDetailSampleMaxError());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("maxedgelength");
  node->SetAttributeAsInt("value", parameters->GetMaxEdgeLength());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("minregionarea");
  node->SetAttributeAsInt("value", parameters->GetMinRegionArea());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("mergeregionarea");
  node->SetAttributeAsInt("value", parameters->GetMergeRegionArea());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("maxvertsperpoly");
  node->SetAttributeAsInt("value", parameters->GetMaxVertsPerPoly());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("tilesize");
  node->SetAttributeAsInt("value", parameters->GetTileSize());

  node = parametersNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("bordersize");
  node->SetAttributeAsInt("value", parameters->GetBorderSize());

  // Navigation mesh header node
  csRef<iDocumentNode> navMeshHeader = mainNode->CreateNodeBefore(CS_NODE_ELEMENT);
  navMeshHeader->SetValue("navmeshsetheader");

  node = navMeshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("magic");
  node->SetAttributeAsInt("value", NAVMESHSET_MAGIC);

  node = navMeshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("version");
  node->SetAttributeAsInt("value", NAVMESHSET_VERSION);

  int numTiles = 0;
  for (int i = 0; i < detourNavMesh->getMaxTiles(); ++i)
  {
    const dtMeshTile* tile = detourNavMesh->getTile(i);
    if (!tile || !tile->header || !tile->dataSize)
    {
      continue;
    }
    numTiles++;
  }
  node = navMeshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("numtiles");
  node->SetAttributeAsInt("value", numTiles);

  csRef<iDocumentNode> paramsNode = navMeshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
  paramsNode->SetValue("dtnavmeshparams");
  const dtNavMeshParams* params = detourNavMesh->getParams();
  node = paramsNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("origin");
  node->SetAttributeAsFloat("x", params->orig[0]);
  node->SetAttributeAsFloat("y", params->orig[1]);
  node->SetAttributeAsFloat("z", params->orig[2]);

  node = paramsNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("tilewidth");
  node->SetAttributeAsFloat("value", params->tileWidth);

  node = paramsNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("tileheight");
  node->SetAttributeAsFloat("value", params->tileHeight);

  node = paramsNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("maxtiles");
  node->SetAttributeAsInt("value", params->maxTiles);

  node = paramsNode->CreateNodeBefore(CS_NODE_ELEMENT);
  node->SetValue("maxpolys");
  node->SetAttributeAsInt("value", params->maxPolys);

  // Create tiles node
  csRef<iDocumentNode> tilesNode = mainNode->CreateNodeBefore(CS_NODE_ELEMENT);
  tilesNode->SetValue("tiles");

  for (int i = 0; i < detourNavMesh->getMaxTiles(); ++i)
  {
    const dtMeshTile* tile = detourNavMesh->getTile(i);
    if (!tile || !tile->header || !tile->dataSize)
    {
      continue;
    }
    csRef<iDocumentNode> tileNode = tilesNode->CreateNodeBefore(CS_NODE_ELEMENT);
    tileNode->SetValue("tile");

    // Create tile header node
    csRef<iDocumentNode> tileHeader = tileNode->CreateNodeBefore(CS_NODE_ELEMENT);
    tileHeader->SetValue("tileheader");
    tileHeader->SetAttributeAsInt("tileref", detourNavMesh->getTileRef(tile));
    tileHeader->SetAttributeAsInt("datasize", tile->dataSize);

    // Create mesh header node
    csRef<iDocumentNode> meshHeader = tileNode->CreateNodeBefore(CS_NODE_ELEMENT);
    meshHeader->SetValue("meshheader");
    unsigned char* data = tile->data;
    dtMeshHeader* header = (dtMeshHeader*)data;

    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("magic");
    node->SetAttributeAsInt("value", header->magic);

    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("version");
    node->SetAttributeAsInt("value", header->version);

    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("location");
    node->SetAttributeAsInt("x", header->x);
    node->SetAttributeAsInt("y", header->y);

    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("userid");
    node->SetAttributeAsInt("value", header->userId);
    
    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("polycount");
    node->SetAttributeAsInt("value", header->polyCount);
    
    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("vertcount");
    node->SetAttributeAsInt("value", header->vertCount);
    
    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("maxlinkcount");
    node->SetAttributeAsInt("value", header->maxLinkCount);
    
    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("detailmeshcount");
    node->SetAttributeAsInt("value", header->detailMeshCount);

    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("detailvertcount");
    node->SetAttributeAsInt("value", header->detailVertCount);
    
    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("detailtricount");
    node->SetAttributeAsInt("value", header->detailTriCount);

    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("bvnodecount");
    node->SetAttributeAsInt("value", header->bvNodeCount);
    
    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("offmeshconcount");
    node->SetAttributeAsInt("value", header->offMeshConCount);

    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("offmeshbase");
    node->SetAttributeAsInt("value", header->offMeshBase);
    
    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("walkableheight");
    node->SetAttributeAsFloat("value", header->walkableHeight);

    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("walkableradius");
    node->SetAttributeAsFloat("value", header->walkableRadius);

    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("walkableclimb");
    node->SetAttributeAsFloat("value", header->walkableClimb);

    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("boundingmin");
    node->SetAttributeAsFloat("x", header->bmin[0]);
    node->SetAttributeAsFloat("y", header->bmin[1]);
    node->SetAttributeAsFloat("z", header->bmin[2]);

    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("boundingmax");
    node->SetAttributeAsFloat("x", header->bmax[0]);
    node->SetAttributeAsFloat("y", header->bmax[1]);
    node->SetAttributeAsFloat("z", header->bmax[2]);

    node = meshHeader->CreateNodeBefore(CS_NODE_ELEMENT);
    node->SetValue("bvquantfactor");
    node->SetAttributeAsFloat("value", header->bvQuantFactor);

    // Create verts node
    csRef<iDocumentNode> vertsNode = tileNode->CreateNodeBefore(CS_NODE_ELEMENT);
    vertsNode->SetValue("verts");
    const int headerSize = dtAlign4(sizeof(dtMeshHeader));
    data += headerSize;
    float* verts = (float*)data;    
    int index = 0;
    for (int j = 0; j < header->vertCount; j++)
    {
      node = vertsNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("vertex");
      node->SetAttributeAsFloat("x", verts[index++]);
      node->SetAttributeAsFloat("y", verts[index++]);
      node->SetAttributeAsFloat("z", verts[index++]);
    }

    // Create polys node
    csRef<iDocumentNode> polysNode = tileNode->CreateNodeBefore(CS_NODE_ELEMENT);
    polysNode->SetValue("polys");
    const int vertsSize = dtAlign4(sizeof(float) * 3 * header->vertCount);
    data += vertsSize;
    dtPoly* polys = (dtPoly*)data;
    index = 0;
    for (int j = 0; j < header->polyCount; j++)
    {
      csRef<iDocumentNode> polyNode = polysNode->CreateNodeBefore(CS_NODE_ELEMENT);
      polyNode->SetValue("poly");

      node = polyNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("firstlink");
      node->SetAttributeAsInt("value", polys[j].firstLink);

      csRef<iDocumentNode> vertsNode2 = polyNode->CreateNodeBefore(CS_NODE_ELEMENT);
      vertsNode2->SetValue("verts");
      csRef<iDocumentNode> neisNode = polyNode->CreateNodeBefore(CS_NODE_ELEMENT);
      neisNode->SetValue("neis");
      for (int k = 0; k < DT_VERTS_PER_POLYGON; k++)
      {
        node = vertsNode2->CreateNodeBefore(CS_NODE_ELEMENT);
        node->SetValue("vertex");
        node->SetAttributeAsInt("indice", polys[j].verts[k]);

        node = neisNode->CreateNodeBefore(CS_NODE_ELEMENT);
        node->SetValue("neighbour");
        node->SetAttributeAsInt("indice", polys[j].neis[k]);
      }

      node = polyNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("flags");
      node->SetAttributeAsInt("value", polys[j].flags);

      node = polyNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("vertcount");
      node->SetAttributeAsInt("value", polys[j].vertCount);

      node = polyNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("area");
      node->SetAttributeAsInt("value", polys[j].getArea());

      node = polyNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("type");
      node->SetAttributeAsInt("value", polys[j].getType());
    }

    // Create links node
    csRef<iDocumentNode> linksNode = tileNode->CreateNodeBefore(CS_NODE_ELEMENT);
    linksNode->SetValue("links");
    const int polysSize = dtAlign4(sizeof(dtPoly) * header->polyCount);
    data += polysSize; 
    dtLink* links = (dtLink*)data;
    for (int j = 0; j < header->maxLinkCount; j++)
    {
      csRef<iDocumentNode> linkNode = linksNode->CreateNodeBefore(CS_NODE_ELEMENT);
      linkNode->SetValue("link");

      node = linkNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("ref");
      node->SetAttributeAsInt("value", links[j].ref);

      node = linkNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("next");
      node->SetAttributeAsInt("value", links[j].next);

      node = linkNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("edge");
      node->SetAttributeAsInt("value", links[j].edge);

      node = linkNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("side");
      node->SetAttributeAsInt("value", links[j].side);

      node = linkNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("bmin");
      node->SetAttributeAsInt("value", links[j].bmin);

      node = linkNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("bmax");
      node->SetAttributeAsInt("value", links[j].bmax);
    }

    // Create detail meshes node
    csRef<iDocumentNode> detailMeshesNode = tileNode->CreateNodeBefore(CS_NODE_ELEMENT);
    detailMeshesNode->SetValue("detailmeshes");
    const int linksSize = dtAlign4(sizeof(dtLink) * (header->maxLinkCount));
    data += linksSize;
    dtPolyDetail* detailMeshes = (dtPolyDetail*)data;
    for (int j = 0; j < header->detailMeshCount; j++)
    {
      csRef<iDocumentNode> detailMeshNode = detailMeshesNode->CreateNodeBefore(CS_NODE_ELEMENT);
      detailMeshNode->SetValue("detailmesh");

      node = detailMeshNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("vertbase");
      node->SetAttributeAsInt("value", detailMeshes[j].vertBase);

      node = detailMeshNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("vertcount");
      node->SetAttributeAsInt("value", detailMeshes[j].vertCount);

      node = detailMeshNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("tribase");
      node->SetAttributeAsInt("value", detailMeshes[j].triBase);

      node = detailMeshNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("tricount");
      node->SetAttributeAsInt("value", detailMeshes[j].triCount);
    }
    
    // Create detail vertices node
    csRef<iDocumentNode> detailVertsNode = tileNode->CreateNodeBefore(CS_NODE_ELEMENT);
    detailVertsNode->SetValue("detailverts");
    const int detailMeshesSize = dtAlign4(sizeof(dtPolyDetail) * header->detailMeshCount);
    data += detailMeshesSize;
    float* detailVerts = (float*)data;
    index = 0;
    for (int j = 0; j < header->detailVertCount; j++)
    {
      node = detailVertsNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("vertex");
      node->SetAttributeAsFloat("x", detailVerts[index++]);
      node->SetAttributeAsFloat("y", detailVerts[index++]);
      node->SetAttributeAsFloat("z", detailVerts[index++]);
    }

    // Create detail triangles node
    csRef<iDocumentNode> detailTrisNode = tileNode->CreateNodeBefore(CS_NODE_ELEMENT);
    detailTrisNode->SetValue("detailtris");
    const int detailVertsSize = dtAlign4(sizeof(float) * 3 * header->detailVertCount);
    data += detailVertsSize;
    unsigned char* detailTris = (unsigned char*)data;
    index = 0;
    for (int j = 0; j < header->detailTriCount; j++)
    {
      node = detailTrisNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("tri");
      node->SetAttributeAsInt("a", detailTris[index++]);
      node->SetAttributeAsInt("b", detailTris[index++]);
      node->SetAttributeAsInt("c", detailTris[index++]);
      node->SetAttributeAsInt("d", detailTris[index++]);
    }
    
    // Create bv tree node
    csRef<iDocumentNode> bvTreeNode = tileNode->CreateNodeBefore(CS_NODE_ELEMENT);
    bvTreeNode->SetValue("bvtree");
    const int detailTrisSize = dtAlign4(sizeof(unsigned char) * 4 * header->detailTriCount);
    data += detailTrisSize;
    dtBVNode* bvTree = (dtBVNode*)data;
    index = 0;
    for (int j = 0; j < header->bvNodeCount; j++)
    {
      csRef<iDocumentNode> bvTreeNodeNode = bvTreeNode->CreateNodeBefore(CS_NODE_ELEMENT);
      bvTreeNodeNode->SetValue("bvtreenode");

      node = bvTreeNodeNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("bmin");
      node->SetAttributeAsInt("x", bvTree[j].bmin[0]);
      node->SetAttributeAsInt("y", bvTree[j].bmin[1]);
      node->SetAttributeAsInt("z", bvTree[j].bmin[2]);

      node = bvTreeNodeNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("bmax");
      node->SetAttributeAsInt("x", bvTree[j].bmax[0]);
      node->SetAttributeAsInt("y", bvTree[j].bmax[1]);
      node->SetAttributeAsInt("z", bvTree[j].bmax[2]);

      node = bvTreeNodeNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("index");
      node->SetAttributeAsInt("value", bvTree[j].i);
    }

    // Create offmesh links node
    csRef<iDocumentNode> offMeshLinksNode = tileNode->CreateNodeBefore(CS_NODE_ELEMENT);
    offMeshLinksNode->SetValue("offmeshlinks");
    const int bvtreeSize = dtAlign4(sizeof(dtBVNode) * header->bvNodeCount);
    data += bvtreeSize;
    dtOffMeshConnection* offMeshCons = (dtOffMeshConnection*)data;
    for (int j = 0; j < header->offMeshConCount; j++)
    {
      csRef<iDocumentNode> offMeshLinkNode = offMeshLinksNode->CreateNodeBefore(CS_NODE_ELEMENT);
      offMeshLinkNode->SetValue("offmeshlink");

      node = offMeshLinkNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("pos");
      node->SetAttributeAsFloat("x1", offMeshCons[j].pos[0]);
      node->SetAttributeAsFloat("y1", offMeshCons[j].pos[1]);
      node->SetAttributeAsFloat("z1", offMeshCons[j].pos[2]);
      node->SetAttributeAsFloat("x2", offMeshCons[j].pos[3]);
      node->SetAttributeAsFloat("y2", offMeshCons[j].pos[4]);
      node->SetAttributeAsFloat("z2", offMeshCons[j].pos[5]);

      node = offMeshLinkNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("rad");
      node->SetAttributeAsFloat("value", offMeshCons[j].rad);

      node = offMeshLinkNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("poly");
      node->SetAttributeAsInt("value", offMeshCons[j].poly);

      node = offMeshLinkNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("flags");
      node->SetAttributeAsInt("value", offMeshCons[j].flags);

      node = offMeshLinkNode->CreateNodeBefore(CS_NODE_ELEMENT);
      node->SetValue("side");
      node->SetAttributeAsInt("value", offMeshCons[j].side);      
    }
  }

  doc->Write(file);

  return true;
}

bool celNavMesh::LoadNavMesh (iFile* file)
{
  csRef<iDocumentSystem> docsys = csLoadPluginCheck<iDocumentSystem>(objectRegistry, "crystalspace.documentsystem.tinyxml");
  if (!docsys)
  {
    return false;
  }
  
  // Read XML file
  csRef<iDocument> doc = docsys->CreateDocument();
  const char* log = doc->Parse(file);
  if (log)
  {
    return false;
  }
  csRef<iDocumentNode> root = doc->GetRoot();
  csRef<iDocumentNode> mainNode = root->GetNode("iCelNavMesh");

  // Get sector
  const char* sectorName = mainNode->GetAttributeValue("sector");
  csString sectorNameString(sectorName);
  csRef<iEngine> engine = csLoadPluginCheck<iEngine>(objectRegistry, "crystalspace.engine.3d");
  if (!engine)
  {
    return false;
  }
  size_t size = engine->GetSectors()->GetCount();
  for (size_t i = 0; i < size; i++)
  {
    csRef<iSector> sector = engine->GetSectors()->Get(i);
    if (sectorNameString == sector->QueryObject()->GetName())
    {
      this->sector = sector;
      break;
    }
  }

  // Read bounding box
  csRef<iDocumentNode> boundingBoxNode = mainNode->GetNode("boundingbox");
  csRef<iDocumentNode> node = boundingBoxNode->GetNode("min");
  boundingMin[0] = node->GetAttributeValueAsFloat("x");
  boundingMin[1] = node->GetAttributeValueAsFloat("y");
  boundingMin[2] = node->GetAttributeValueAsFloat("z");
  node = boundingBoxNode->GetNode("max");
  boundingMax[0] = node->GetAttributeValueAsFloat("x");
  boundingMax[1] = node->GetAttributeValueAsFloat("y");
  boundingMax[2] = node->GetAttributeValueAsFloat("z");

  // Read parameters
  this->parameters.AttachNew(new celNavMeshParams());
  csRef<iDocumentNode> parametersNode = mainNode->GetNode("parameters");
  node = parametersNode->GetNode("agentheight");
  float value = node->GetAttributeValueAsFloat("value");
  parameters->SetAgentHeight(value);
  node = parametersNode->GetNode("agentradius");
  value = node->GetAttributeValueAsFloat("value");
  parameters->SetAgentRadius(value);
  node = parametersNode->GetNode("agentmaxslopeangle");
  value = node->GetAttributeValueAsFloat("value");
  parameters->SetAgentMaxSlopeAngle(value);
  node = parametersNode->GetNode("agentmaxclimb");
  value = node->GetAttributeValueAsFloat("value");
  parameters->SetAgentMaxClimb(value);
  node = parametersNode->GetNode("cellsize");
  value = node->GetAttributeValueAsFloat("value");
  parameters->SetCellSize(value);
  node = parametersNode->GetNode("cellheight");
  value = node->GetAttributeValueAsFloat("value");
  parameters->SetCellHeight(value);
  node = parametersNode->GetNode("maxsimplificationerror");
  value = node->GetAttributeValueAsFloat("value");
  parameters->SetMaxSimplificationError(value);
  node = parametersNode->GetNode("detailsampledist");
  value = node->GetAttributeValueAsFloat("value");
  parameters->SetDetailSampleDist(value);
  node = parametersNode->GetNode("detailsamplemaxerror");
  value = node->GetAttributeValueAsFloat("value");
  parameters->SetDetailSampleMaxError(value);
  node = parametersNode->GetNode("maxedgelength");
  int value2 = node->GetAttributeValueAsInt("value");
  parameters->SetMaxEdgeLength(value2);
  node = parametersNode->GetNode("minregionarea");
  value2 = node->GetAttributeValueAsInt("value");
  parameters->SetMinRegionArea(value2);
  node = parametersNode->GetNode("mergeregionarea");
  value2 = node->GetAttributeValueAsInt("value");
  parameters->SetMergeRegionArea(value2);
  node = parametersNode->GetNode("maxvertsperpoly");
  value2 = node->GetAttributeValueAsInt("value");
  parameters->SetMaxVertsPerPoly(value2);
  node = parametersNode->GetNode("tilesize");
  value2 = node->GetAttributeValueAsInt("value");
  parameters->SetTileSize(value2);
  node = parametersNode->GetNode("bordersize");
  value2 = node->GetAttributeValueAsInt("value");
  parameters->SetBorderSize(value2);

  // Read header
  NavMeshSetHeader header;
  csRef<iDocumentNode> navMeshHeaderNode = mainNode->GetNode("navmeshsetheader");
  node = navMeshHeaderNode->GetNode("magic");
  header.magic = node->GetAttributeValueAsInt("value");
  node = navMeshHeaderNode->GetNode("version");
  header.version = node->GetAttributeValueAsInt("value");
  node = navMeshHeaderNode->GetNode("numtiles");
  header.numTiles = node->GetAttributeValueAsInt("value");
  csRef<iDocumentNode> paramsNode = navMeshHeaderNode->GetNode("dtnavmeshparams");
  node = paramsNode->GetNode("origin");
  header.params.orig[0] = node->GetAttributeValueAsFloat("x");
  header.params.orig[1] = node->GetAttributeValueAsFloat("y");
  header.params.orig[2] = node->GetAttributeValueAsFloat("z");
  node = paramsNode->GetNode("tilewidth");
  header.params.tileWidth = node->GetAttributeValueAsFloat("value");
  node = paramsNode->GetNode("tileheight");
  header.params.tileHeight = node->GetAttributeValueAsFloat("value");
  node = paramsNode->GetNode("maxtiles");
  header.params.maxTiles = node->GetAttributeValueAsInt("value");
  node = paramsNode->GetNode("maxpolys");
  header.params.maxPolys = node->GetAttributeValueAsInt("value");

  if (header.magic != NAVMESHSET_MAGIC)
  {
    return false;
  }
  if (header.version != NAVMESHSET_VERSION)
  {
    return false;
  }
  detourNavMesh = new dtNavMesh;
  if (!detourNavMesh || !detourNavMesh->init(&header.params))
  {
    return false;
  }
  detourNavMeshQuery = new dtNavMeshQuery;
  if (!detourNavMeshQuery || !detourNavMeshQuery->init(detourNavMesh, MAX_NODES))
  {
    return false;
  }
  
  // Read tiles
  csRef<iDocumentNode> tilesNode = mainNode->GetNode("tiles");
  csRef<iDocumentNodeIterator> tileNodes = tilesNode->GetNodes("tile");
  while (tileNodes->HasNext())
  {
    csRef<iDocumentNode> tileNode = tileNodes->Next();

    // Read tile header
    NavMeshTileHeader tileHeader;
    csRef<iDocumentNode> tileHeaderNode = tileNode->GetNode("tileheader");
    tileHeader.tileRef = (unsigned int)tileHeaderNode->GetAttributeValueAsInt("tileref");
    tileHeader.dataSize = tileHeaderNode->GetAttributeValueAsInt("datasize");

    if (!tileHeader.tileRef || !tileHeader.dataSize)
    {
      break;
    }
    unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
    if (!data)
    {
      break;
    }
    memset(data, 0, tileHeader.dataSize);

    // Read mesh header
    dtMeshHeader* meshHeader = (dtMeshHeader*)data;
    csRef<iDocumentNode> meshHeaderNode = tileNode->GetNode("meshheader");
    node = meshHeaderNode->GetNode("magic");
    meshHeader->magic = node->GetAttributeValueAsInt("value");
    node = meshHeaderNode->GetNode("version");
    meshHeader->version = node->GetAttributeValueAsInt("value");
    node = meshHeaderNode->GetNode("location");
    meshHeader->x = node->GetAttributeValueAsInt("x");
    meshHeader->y = node->GetAttributeValueAsInt("y");
    node = meshHeaderNode->GetNode("userid");
    meshHeader->userId = (unsigned int)node->GetAttributeValueAsInt("value");
    node = meshHeaderNode->GetNode("polycount");
    meshHeader->polyCount = node->GetAttributeValueAsInt("value");
    node = meshHeaderNode->GetNode("vertcount");
    meshHeader->vertCount = node->GetAttributeValueAsInt("value");
    node = meshHeaderNode->GetNode("maxlinkcount");
    meshHeader->maxLinkCount = node->GetAttributeValueAsInt("value");
    node = meshHeaderNode->GetNode("detailmeshcount");
    meshHeader->detailMeshCount = node->GetAttributeValueAsInt("value");
    node = meshHeaderNode->GetNode("detailvertcount");
    meshHeader->detailVertCount = node->GetAttributeValueAsInt("value");
    node = meshHeaderNode->GetNode("detailtricount");
    meshHeader->detailTriCount = node->GetAttributeValueAsInt("value");
    node = meshHeaderNode->GetNode("bvnodecount");
    meshHeader->bvNodeCount = node->GetAttributeValueAsInt("value");
    node = meshHeaderNode->GetNode("offmeshconcount");
    meshHeader->offMeshConCount = node->GetAttributeValueAsInt("value");
    node = meshHeaderNode->GetNode("offmeshbase");
    meshHeader->offMeshBase = node->GetAttributeValueAsInt("value");
    node = meshHeaderNode->GetNode("walkableheight");
    meshHeader->walkableHeight = node->GetAttributeValueAsFloat("value");
    node = meshHeaderNode->GetNode("walkableradius");
    meshHeader->walkableRadius = node->GetAttributeValueAsFloat("value");
    node = meshHeaderNode->GetNode("walkableclimb");
    meshHeader->walkableClimb = node->GetAttributeValueAsFloat("value");
    node = meshHeaderNode->GetNode("boundingmin");
    meshHeader->bmin[0] = node->GetAttributeValueAsFloat("x");
    meshHeader->bmin[1] = node->GetAttributeValueAsFloat("y");
    meshHeader->bmin[2] = node->GetAttributeValueAsFloat("z");
    node = meshHeaderNode->GetNode("boundingmax");
    meshHeader->bmax[0] = node->GetAttributeValueAsFloat("x");
    meshHeader->bmax[1] = node->GetAttributeValueAsFloat("y");
    meshHeader->bmax[2] = node->GetAttributeValueAsFloat("z");
    node = meshHeaderNode->GetNode("bvquantfactor");
    meshHeader->bvQuantFactor = node->GetAttributeValueAsFloat("value");

    // Read verts node
    const int headerSize = dtAlign4(sizeof(dtMeshHeader));
    unsigned char* d = data + headerSize;
    float* verts = (float*)d;
    csRef<iDocumentNode> vertsNode = tileNode->GetNode("verts");
    csRef<iDocumentNodeIterator> vertices = vertsNode->GetNodes("vertex");
    int index = 0;
    while (vertices->HasNext())
    {
      csRef<iDocumentNode> vertex = vertices->Next();
      verts[index++] = vertex->GetAttributeValueAsFloat("x");
      verts[index++] = vertex->GetAttributeValueAsFloat("y");
      verts[index++] = vertex->GetAttributeValueAsFloat("z");
    }

    // Read polys node
    const int vertsSize = dtAlign4(sizeof(float) * 3 * meshHeader->vertCount);
    d += vertsSize;    
    dtPoly* polys = (dtPoly*)d;
    csRef<iDocumentNode> polysNode = tileNode->GetNode("polys");
    csRef<iDocumentNodeIterator> polyNodes = polysNode->GetNodes("poly");
    index = 0;
    while (polyNodes->HasNext())
    {
      csRef<iDocumentNode> polyNode = polyNodes->Next();
      node = polyNode->GetNode("firstlink");
      polys[index].firstLink = (unsigned int)node->GetAttributeValueAsInt("value");
      csRef<iDocumentNode> vertsNode = polyNode->GetNode("verts");
      csRef<iDocumentNodeIterator> verts = vertsNode->GetNodes("vertex");
      int index2 = 0;
      while (verts->HasNext())
      {
        polys[index].verts[index2++] = (unsigned short)verts->Next()->GetAttributeValueAsInt("indice");
      }
      csRef<iDocumentNode> neisNode = polyNode->GetNode("neis");
      csRef<iDocumentNodeIterator> neighbours = neisNode->GetNodes("neighbour");
      index2 = 0;
      while (neighbours->HasNext())
      {
        polys[index].neis[index2++] = (unsigned short)neighbours->Next()->GetAttributeValueAsInt("indice");;
      }
      node = polyNode->GetNode("flags");
      polys[index].flags = (unsigned short)node->GetAttributeValueAsInt("value");
      node = polyNode->GetNode("vertcount");
      polys[index].vertCount = (unsigned char)node->GetAttributeValueAsInt("value");
      node = polyNode->GetNode("area");
      polys[index].setArea((unsigned char)node->GetAttributeValueAsInt("value"));
      node = polyNode->GetNode("type");
      polys[index].setType((unsigned char)node->GetAttributeValueAsInt("value"));
      index++;
    }

    // Read links node
    const int polysSize = dtAlign4(sizeof(dtPoly) * meshHeader->polyCount);
    d += polysSize;    
    dtLink* links = (dtLink*)d;
    csRef<iDocumentNode> linksNode = tileNode->GetNode("links");
    csRef<iDocumentNodeIterator> linksNodes = linksNode->GetNodes("link");
    index = 0;
    while (linksNodes->HasNext())
    {
      csRef<iDocumentNode> linkNode = linksNodes->Next();
      node = linkNode->GetNode("ref");
      links[index].ref = (unsigned int)node->GetAttributeValueAsInt("value");
      node = linkNode->GetNode("next");
      links[index].next = (unsigned int)node->GetAttributeValueAsInt("value");
      node = linkNode->GetNode("edge");
      links[index].edge = (unsigned char)node->GetAttributeValueAsInt("value");
      node = linkNode->GetNode("side");
      links[index].side = (unsigned char)node->GetAttributeValueAsInt("value");
      node = linkNode->GetNode("bmin");
      links[index].bmin = (unsigned char)node->GetAttributeValueAsInt("value");
      node = linkNode->GetNode("bmax");
      links[index].bmax = (unsigned char)node->GetAttributeValueAsInt("value");
      index++;
    }

    // Read detail meshes node
    const int linksSize = dtAlign4(sizeof(dtLink) * (meshHeader->maxLinkCount));
    d += linksSize;
    dtPolyDetail* detailMeshes = (dtPolyDetail*)d;
    csRef<iDocumentNode> detailMeshesNode = tileNode->GetNode("detailmeshes");
    csRef<iDocumentNodeIterator> detailMeshesNodes = detailMeshesNode->GetNodes("detailmesh");
    index = 0;
    while (detailMeshesNodes->HasNext())
    {
      csRef<iDocumentNode> detailMeshNode = detailMeshesNodes->Next();
      node = detailMeshNode->GetNode("vertbase");
      detailMeshes[index].vertBase = (unsigned short)node->GetAttributeValueAsInt("value");
      node = detailMeshNode->GetNode("vertcount");
      detailMeshes[index].vertCount = (unsigned short)node->GetAttributeValueAsInt("value");
      node = detailMeshNode->GetNode("tribase");
      detailMeshes[index].triBase = (unsigned short)node->GetAttributeValueAsInt("value");
      node = detailMeshNode->GetNode("tricount");
      detailMeshes[index].triCount = (unsigned short)node->GetAttributeValueAsInt("value");
      index++;
    }

    // Read detail vertices node
    const int detailMeshesSize = dtAlign4(sizeof(dtPolyDetail) * meshHeader->detailMeshCount);
    d += detailMeshesSize;
    float* detailVerts = (float*)d;
    csRef<iDocumentNode> detailVertsNode = tileNode->GetNode("detailverts");
    csRef<iDocumentNodeIterator> vertices2 = detailVertsNode->GetNodes("vertex");
    index = 0;
    while (vertices2->HasNext())
    {
      csRef<iDocumentNode> vertex = vertices2->Next();
      detailVerts[index++] = vertex->GetAttributeValueAsFloat("x");
      detailVerts[index++] = vertex->GetAttributeValueAsFloat("y");
      detailVerts[index++] = vertex->GetAttributeValueAsFloat("z");
    }

    // Read detail tris node
    const int detailVertsSize = dtAlign4(sizeof(float) * 3 * meshHeader->detailVertCount);
    d += detailVertsSize;
    unsigned char* detailTris = (unsigned char*)d;
    csRef<iDocumentNode> detailTrisNode = tileNode->GetNode("detailtris");
    csRef<iDocumentNodeIterator> triangles = detailTrisNode->GetNodes("tri");
    index = 0;
    while (triangles->HasNext())
    {
      csRef<iDocumentNode> tri = triangles->Next();
      detailTris[index++] = (unsigned char)tri->GetAttributeValueAsInt("a");
      detailTris[index++] = (unsigned char)tri->GetAttributeValueAsInt("b");
      detailTris[index++] = (unsigned char)tri->GetAttributeValueAsInt("c");
      detailTris[index++] = (unsigned char)tri->GetAttributeValueAsInt("d");
    }

    // Read bv tree node
    const int detailTrisSize = dtAlign4(sizeof(unsigned char) * 4 * meshHeader->detailTriCount);
    d += detailTrisSize;
    dtBVNode* bvTree = (dtBVNode*)d;
    csRef<iDocumentNode> bvTreeNode = tileNode->GetNode("bvtree");
    csRef<iDocumentNodeIterator> bvTreeNodeNodes = bvTreeNode->GetNodes("bvtreenode");
    index = 0;
    while (bvTreeNodeNodes->HasNext())
    {
      csRef<iDocumentNode> bvTreeNodeNode = bvTreeNodeNodes->Next();
      node = bvTreeNodeNode->GetNode("bmin");
      bvTree[index].bmin[0] = (unsigned short)node->GetAttributeValueAsInt("x");
      bvTree[index].bmin[1] = (unsigned short)node->GetAttributeValueAsInt("y");
      bvTree[index].bmin[2] = (unsigned short)node->GetAttributeValueAsInt("z");
      
      node = bvTreeNodeNode->GetNode("bmax");
      bvTree[index].bmax[0] = (unsigned short)node->GetAttributeValueAsInt("x");
      bvTree[index].bmax[1] = (unsigned short)node->GetAttributeValueAsInt("y");
      bvTree[index].bmax[2] = (unsigned short)node->GetAttributeValueAsInt("z");

      node = bvTreeNodeNode->GetNode("index");
      bvTree[index].i = node->GetAttributeValueAsInt("value");
      index++;
    }

    // Read off mesh links node
    const int bvtreeSize = dtAlign4(sizeof(dtBVNode) * meshHeader->bvNodeCount);
    d += bvtreeSize;
    dtOffMeshConnection* offMeshCons = (dtOffMeshConnection*)d;
    csRef<iDocumentNode> offMeshLinksNode = tileNode->GetNode("offmeshlinks");
    csRef<iDocumentNodeIterator> offMeshLinksNodes = offMeshLinksNode->GetNodes("offmeshlink");
    index = 0;
    while (offMeshLinksNodes->HasNext())
    {
      csRef<iDocumentNode> offMeshLinkNode = offMeshLinksNodes->Next();
      node = offMeshLinkNode->GetNode("pos");
      offMeshCons[index].pos[0] = node->GetAttributeValueAsFloat("x1");
      offMeshCons[index].pos[1] = node->GetAttributeValueAsFloat("y1");
      offMeshCons[index].pos[2] = node->GetAttributeValueAsFloat("z1");
      offMeshCons[index].pos[3] = node->GetAttributeValueAsFloat("x2");
      offMeshCons[index].pos[4] = node->GetAttributeValueAsFloat("y2");
      offMeshCons[index].pos[5] = node->GetAttributeValueAsFloat("z2");

      node = offMeshLinkNode->GetNode("rad");
      offMeshCons[index].rad = node->GetAttributeValueAsFloat("value");

      node = offMeshLinkNode->GetNode("poly");
      offMeshCons[index].poly = (unsigned short)node->GetAttributeValueAsInt("value");

      node = offMeshLinkNode->GetNode("flags");
      offMeshCons[index].flags = (unsigned char)node->GetAttributeValueAsInt("value");

      node = offMeshLinkNode->GetNode("side");
      offMeshCons[index].side = (unsigned char)node->GetAttributeValueAsInt("value");
      index++;
    }
    dtTileRef result;
    detourNavMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, &result);
    if (!result)
    {
      return false;
    }
  }

  return true;
}


csArray<csSimpleRenderMesh*>* celNavMesh::GetDebugMeshes () 
{
  // Clear previous debug meshes
  if (!debugMeshes->IsEmpty()) 
  { 
    csArray<csSimpleRenderMesh*>::Iterator it = debugMeshes->GetIterator(); 
    while (it.HasNext()) 
    { 
      csSimpleRenderMesh* mesh = it.Next(); 
      delete [] mesh->vertices; 
      delete [] mesh->colors; 
    }
    debugMeshes->DeleteAll(); 
  }

  // Update debug meshes
  DebugDrawCS dd;
  duDebugDrawNavMesh(&dd, *detourNavMesh, navMeshDrawFlags);
  debugMeshes = dd.GetMeshes();
  return debugMeshes;
}

csArray<csSimpleRenderMesh*>* celNavMesh::GetAgentDebugMeshes (const csVector3& pos) 
{
  return GetAgentDebugMeshes(pos, 51, 102, 0, 129);
}

csArray<csSimpleRenderMesh*>* celNavMesh::GetAgentDebugMeshes (const csVector3& pos, int red, int green, 
                                                             int blue, int alpha) 
{
  // Update agent debug meshes
  DebugDrawCS dd;
  dd.depthMask (false);

  const float r = parameters->GetAgentRadius();
  const float h = parameters->GetAgentHeight();
  const float c = parameters->GetAgentMaxClimb();
  const unsigned int col = duRGBA(red, green, blue, alpha);

  // Agent dimensions.	
  duDebugDrawCylinderWire(&dd, pos[0] - r, pos[1] + 0.02f, pos[2] - r, pos[0] + r, pos[1] + h, pos[2] + r, col, 4.0f);
  duDebugDrawCircle(&dd, pos[0], pos[1] + c, pos[2], r, duRGBA(0, 0, 0, 180), 4.0f);

  unsigned int colb = duRGBA(0, 0, 0, 180);
  dd.begin(DU_DRAW_LINES, 2.0f);
  dd.vertex(pos[0], pos[1] - c, pos[2], colb);
  dd.vertex(pos[0], pos[1] + c, pos[2], colb);
  dd.vertex(pos[0] - r/2, pos[1] + 0.02f, pos[2], colb);
  dd.vertex(pos[0] + r/2, pos[1] + 0.02f, pos[2], colb);
  dd.vertex(pos[0], pos[1] + 0.02f, pos[2] - r/2, colb);
  dd.vertex(pos[0], pos[1] + 0.02f, pos[2] + r/2, colb);
  dd.end();

  dd.depthMask (true);

  // Add this new proxy agent to any existing
  csArray<csSimpleRenderMesh*>* tmp = dd.GetMeshes();
  csArray<csSimpleRenderMesh*>::Iterator tmpIt = tmp->GetIterator();
  while (tmpIt.HasNext())
  {
    agentDebugMeshes->Push(tmpIt.Next());
  }
  return agentDebugMeshes;
}

void celNavMesh::ResetAgentDebugMeshes ()
{
  // Clear previous agent debug meshes
  if (!agentDebugMeshes->IsEmpty()) 
  { 
    csArray<csSimpleRenderMesh*>::Iterator it = agentDebugMeshes->GetIterator(); 
    while (it.HasNext()) 
    { 
      csSimpleRenderMesh* mesh = it.Next(); 
      delete [] mesh->vertices; 
      delete [] mesh->colors; 
    }
    agentDebugMeshes->DeleteAll(); 
  }
}

/*
 * celNavMeshParams
 */

celNavMeshParams::celNavMeshParams () : scfImplementationType (this)
{
  SetSuggestedValues(2.0f, 0.6f, 45);
}

celNavMeshParams::celNavMeshParams (const iCelNavMeshParams* parameters) : scfImplementationType (this)
{
  agentHeight = parameters->GetAgentHeight();
  agentRadius = parameters->GetAgentRadius();
  agentMaxSlopeAngle = parameters->GetAgentMaxSlopeAngle();
  agentMaxClimb = parameters->GetAgentMaxClimb();
  cellSize = parameters->GetCellSize();
  cellHeight = parameters->GetCellHeight();
  maxSimplificationError = parameters->GetMaxSimplificationError();
  detailSampleDist = parameters->GetDetailSampleDist();
  detailSampleMaxError = parameters->GetDetailSampleMaxError();
  maxEdgeLength = parameters->GetMaxEdgeLength();
  minRegionArea = parameters->GetMinRegionArea();
  mergeRegionArea = parameters->GetMergeRegionArea();
  maxVertsPerPoly = parameters->GetMaxVertsPerPoly();
  tileSize = parameters->GetTileSize();
  borderSize = parameters->GetBorderSize();
  polygonSearchBox = parameters->GetPolygonSearchBox();
}

celNavMeshParams::~celNavMeshParams ()
{
}

iCelNavMeshParams* celNavMeshParams::Clone () const
{
  celNavMeshParams* params = new celNavMeshParams(this);
  return params;
}

void celNavMeshParams::SetSuggestedValues (float agentHeight, float agentRadius, float agentMaxSlopeAngle)
{
  this->agentHeight = agentHeight;
  this->agentRadius = agentRadius;
  this->agentMaxSlopeAngle = agentMaxSlopeAngle;
  agentMaxClimb = agentHeight / 4.0f;
  cellSize = agentRadius / 2.0f;
  cellHeight = cellSize / 2.0f;
  maxSimplificationError = 1.3f;
  detailSampleDist = 6.0f;
  detailSampleMaxError = 1.0f;
  maxEdgeLength = ((int)ceilf(agentRadius / cellSize)) * 8;  
  minRegionArea = 20;
  mergeRegionArea = 50;
  maxVertsPerPoly = 6;
  tileSize = 32;
  borderSize = (int)ceilf(agentRadius / cellSize) + 3;
  polygonSearchBox = csVector3(2, 4, 2); 
}

float celNavMeshParams::GetAgentHeight () const
{
  return agentHeight;
}

void celNavMeshParams::SetAgentHeight (const float height)
{
  agentHeight = height;
}

float celNavMeshParams::GetAgentRadius () const
{
  return agentRadius;
}

void celNavMeshParams::SetAgentRadius (const float radius)
{
  agentRadius = radius;
}

float celNavMeshParams::GetAgentMaxSlopeAngle () const 
{
  return agentMaxSlopeAngle;
}

void celNavMeshParams::SetAgentMaxSlopeAngle (const float angle)
{
  agentMaxSlopeAngle = angle;
}

float celNavMeshParams::GetAgentMaxClimb () const
{
  return agentMaxClimb;
}

void celNavMeshParams::SetAgentMaxClimb (const float climb)
{
  agentMaxClimb = climb;
}

float celNavMeshParams::GetCellSize () const
{
  return cellSize;
}

void celNavMeshParams::SetCellSize (const float size)
{
  cellSize = size;
}

float celNavMeshParams::GetCellHeight () const
{
  return cellHeight;
}

void celNavMeshParams::SetCellHeight (const float height)
{
  cellHeight = height;
}

float celNavMeshParams::GetMaxSimplificationError () const
{
  return maxSimplificationError;
}

void celNavMeshParams::SetMaxSimplificationError (const float error)
{
  maxSimplificationError = error;
}

float celNavMeshParams::GetDetailSampleDist () const
{
  return detailSampleDist;
}

void celNavMeshParams::SetDetailSampleDist (const float dist)
{
  detailSampleDist = dist;
}

float celNavMeshParams::GetDetailSampleMaxError () const
{
  return detailSampleMaxError;
}

void celNavMeshParams::SetDetailSampleMaxError (const float error)
{
  detailSampleMaxError = error;
}

int celNavMeshParams::GetMaxEdgeLength () const
{
  return maxEdgeLength;
}

void celNavMeshParams::SetMaxEdgeLength (const int length)
{
  maxEdgeLength = length;
}

int celNavMeshParams::GetMinRegionArea () const
{
  return minRegionArea;
}

void celNavMeshParams::SetMinRegionArea (const int area)
{
  minRegionArea = area;
}

int celNavMeshParams::GetMergeRegionArea () const
{
  return mergeRegionArea;
}

void celNavMeshParams::SetMergeRegionArea (const int area)
{
  mergeRegionArea = area;
}

int celNavMeshParams::GetMaxVertsPerPoly () const
{
  return maxVertsPerPoly;
}

void celNavMeshParams::SetMaxVertsPerPoly (const int maxVerts)
{
  maxVertsPerPoly = maxVerts;
}

int celNavMeshParams::GetTileSize () const
{
  return tileSize;
}

void celNavMeshParams::SetTileSize (const int size)
{
  tileSize = size;
}

int celNavMeshParams::GetBorderSize () const
{
  return borderSize;
}

void celNavMeshParams::SetBorderSize (const int size)
{
  borderSize = size;
}

csVector3 celNavMeshParams::GetPolygonSearchBox () const
{
  return polygonSearchBox;
}

void celNavMeshParams::SetPolygonSearchBox (const csVector3 box)
{
  polygonSearchBox = box;
}



/*
 * celNavMeshBuilder
 */

SCF_IMPLEMENT_FACTORY (celNavMeshBuilder)

celNavMeshBuilder::celNavMeshBuilder (iBase* parent) : scfImplementationType (this, parent)
{
  // Pointers
  triangleVertices = 0;
  triangleIndices = 0;
  chunkyTriMesh = 0;
  triangleAreas = 0;
  solid = 0;
  chf = 0;
  cSet = 0;
  pMesh = 0;
  dMesh = 0;

  numberOfVertices = 0;
  numberOfTriangles = 0;
  numberOfOffMeshCon = 0;
  numberOfVolumes = 0;

  parameters.AttachNew(new celNavMeshParams());
}

celNavMeshBuilder::~celNavMeshBuilder ()
{
  CleanUpSectorData();
  CleanUpTileData();
}

void celNavMeshBuilder::CleanUpSectorData () 
{
  delete triangleVertices;
  triangleVertices = 0;
  delete triangleIndices;
  triangleIndices = 0;
  delete chunkyTriMesh;
  chunkyTriMesh = 0;

  numberOfVertices = 0;
  numberOfTriangles = 0;
  numberOfOffMeshCon = 0;
  numberOfVolumes = 0;
}

bool celNavMeshBuilder::Initialize (iObjectRegistry* objectRegistry) 
{
  this->objectRegistry = objectRegistry;
  strings = csQueryRegistryTagInterface<iStringSet>(objectRegistry, "crystalspace.shared.stringset");
  if (!strings)
  {
    return csApplicationFramework::ReportError("Failed to locate the standard stringset!");
  }
  return true;
}

bool celNavMeshBuilder::SetSector (iSector* sector) {
  CleanUpSectorData();
  currentSector = sector;
  return GetSectorData();
}

bool celNavMeshBuilder::CheckClipping(const csPlane3& clipPlane, const csBox3& bbox, bool& result)
{
  // test whether we really have to clip
  result = true;
  if(!csIntersect3::BoxPlane(bbox, clipPlane))
  {
    result = false;
    // bbox doesn't intersect with the plane, check whether we have to process it
    if(clipPlane.Classify(bbox.GetCenter()) >= 0)
    {
      return false;
    }
  }
  return true;
}

void celNavMeshBuilder::SplitPolygon(int indexOffset, int vertCount, csVector3* verts, csArray<csVector3>& vertices, csArray<csTriangle>& triangles, const csReversibleTransform& transform, csPlane3& clipPlane, bool clipPolygon)
{
  // do the intersection
  size_t numVerts = vertCount;
  if(clipPolygon && !clipPlane.ClipPolygon(verts, vertCount))
  {
    // skip invisible polys if clipping is enabled
    return;
  }

  size_t i;
  if(!transform.IsIdentity())
  {
    for(i = 0; i < numVerts; ++i)
    {
      verts[i] = transform.This2Other(verts[i]);
    }
  }

  // get indices and add vertices if not yet present
  size_t* indices = new size_t[numVerts];
  for(i = 0; i < numVerts; ++i)
  {
    indices[i] = vertices.Find(verts[i]);
    if(indices[i] == csArrayItemNotFound)
    {
      indices[i] = vertices.GetSize();
      vertices.Push(verts[i]);
    }
    indices[i] += indexOffset;
  }

  // add new triangles
  csTriangle tri;
  for(i = 2; i < numVerts; i++)
  {
    tri.Set(indices[0],indices[i-1],indices[i]);
    triangles.Push(tri);
  }

  // free temporary buffer
  delete [] indices;
}

/*
 * This method gets the triangles for all the meshes in the sector and stores them, in order to
 * be able to build the navigation meshes later.
 */
// Based on Recast InputGeom::loadMesh
bool celNavMeshBuilder::GetSectorData () 
{
  csArray<csTriangle> triangles;
  csArray<csVector3> vertices;
  csStringID base = strings->Request("base");
  csStringID collDet = strings->Request("colldet");
  
  csVector3 v;
  csRefArray<iMeshWrapper> meshList;
  csHash<csRef<iPortal>,csPtrKey<iMeshWrapper> > portals;

  // add nearby meshes from other sectors
  {
    csRef<iEngine> engine = csQueryRegistry<iEngine>(objectRegistry);
    csSet<csPtrKey<iMeshWrapper> >::GlobalIterator portalMeshesIt = currentSector->GetPortalMeshes().GetIterator();
    while(portalMeshesIt.HasNext())
    {
      csRef<iPortalContainer> container = portalMeshesIt.Next()->GetPortalContainer();
      int size = container->GetPortalCount();
      for (int i = 0; i < size; i++)
      {
        csRef<iPortal> portal = container->GetPortal(i);
        portal->CompleteSector(0);

        if (portal->GetVertexIndicesCount() >= 3 && portal->GetSector())
        {
          // Calculate portal's bounding box
          csBox3 bbox;
          {
            // Get portal vertices
            int verticesSize = portal->GetVerticesCount();
            const csVector3* verticesPortal = portal->GetWorldVertices();

            // add vertices to bbox
            bbox.AddBoundingVertex(verticesPortal[0]);
            for (int j = 1; j < verticesSize; j++)
            {
              bbox.AddBoundingVertexSmart(verticesPortal[j]);
            }
          }

          // get the clipping plane
          csPlane3 clipPlane = portal->GetWorldPlane();

          // move the bbox slightly in front of portal so it will be traversed
          csVector3 portalOrigin = bbox.GetCenter();
          csVector3 portalNormal = clipPlane.Normal();
          portalOrigin += 0.1*portalNormal;
          bbox.SetCenter(portalOrigin);

          // extend the bbox by PolygonSearchBox + 2*AgentSize
          csVector3 size = bbox.GetSize();
          size += parameters->GetPolygonSearchBox();
          size += csVector3(parameters->GetAgentRadius()*2,
                            parameters->GetAgentHeight(),
                            parameters->GetAgentRadius()*2);
          size += csVector3(parameters->GetBorderSize());
          size += csVector3(parameters->GetTileSize());
          bbox.SetSize(size);

          // warp the clipping plane and bbox so we can use them in the target sector
          if(portal->GetFlags().Check(CS_PORTAL_WARP))
          {
            clipPlane = portal->GetWarp().Other2This(clipPlane);
            bbox = portal->GetWarp().Other2This(bbox);
          }

          // add meshes of the sector this portal leads to
          csRef<iMeshList> sectorList = portal->GetSector()->GetMeshes();
          int numberOfMeshes = sectorList->GetCount();

          for (int i = 0; i < numberOfMeshes; i++) 
          {
            csRef<iMeshWrapper> meshWrapper = sectorList->Get(i);
            csBox3 meshBBox = meshWrapper->GetWorldBoundingBox();
            if(bbox.TestIntersect(meshBBox) && (csIntersect3::BoxPlane(meshBBox, clipPlane) || clipPlane.Classify(meshBBox.GetCenter()) < 0))
            {
              meshList.Push(meshWrapper);
              portals.Put(csPtrKey<iMeshWrapper>(meshWrapper), portal);
            }
          }
        }
      }
    }
  }

  csPrintf("added %zu meshes from other sectors to %s\n", meshList.GetSize(), currentSector->QueryObject()->GetName());

  // add meshes in the sector
  {
    csRef<iMeshList> sectorList = currentSector->GetMeshes();
    int numberOfMeshes = sectorList->GetCount();

    for (int i = 0; i < numberOfMeshes; i++) 
    {
      meshList.Push(sectorList->Get(i));
    }
  }

  csRefArray<iMeshWrapper>::Iterator it(meshList.GetIterator());
  while(it.HasNext())
  {
    csRef<iMeshWrapper> meshWrapper(it.Next());
    if(meshWrapper->GetFlags().Check(CS_ENTITY_INVISIBLEMESH|CS_ENTITY_CAMERA)
       || meshWrapper->GetPortalContainer())
    {
      // we don't want to take this mesh into consideration for our navmesh
      continue;
    }

    // obtain collision data
    csRef<iObjectModel> objectModel = meshWrapper->GetMeshObject()->GetObjectModel();
    csRef<iTerrainSystem> terrainSystem = scfQueryInterface<iTerrainSystem>(meshWrapper->GetMeshObject());
    csRef<iTriangleMesh> triangleMesh;
    if (objectModel->IsTriangleDataSet(collDet))
    {
      triangleMesh = objectModel->GetTriangleData(collDet);
    }
    else 
    {
      triangleMesh = objectModel->GetTriangleData(base);
    }

    // skip meshes with empty collision data
    if(!triangleMesh.IsValid() && !terrainSystem.IsValid())
    {
      continue;
    }

    csReversibleTransform transform = meshWrapper->GetMovable()->GetFullTransform();
    csRef<iPortal> portal;
    csPlane3 clipPlane;
    bool needsClipping = false;
    if(portals.Contains(csPtrKey<iMeshWrapper>(meshWrapper)))
    {
      portal = portals.Get(csPtrKey<iMeshWrapper>(meshWrapper),0);
      if(portal->GetFlags().Check(CS_PORTAL_WARP))
      {
        transform *= portal->GetWarp();
      }
      needsClipping = portal->GetFlags().Check(CS_PORTAL_CLIPDEST
                           | CS_PORTAL_CLIPSTRADDLING | CS_PORTAL_ZFILL
                           | CS_PORTAL_MIRROR | CS_PORTAL_FLOAT);
    }
    needsClipping &= !meshWrapper->GetFlags().Check(CS_ENTITY_NOCLIP);

    if(needsClipping)
    {
      // transform clipping plane into object space
      clipPlane = transform.This2Other(portal->GetWorldPlane());

      // check whether the object is visible and really needs clipping
      if(!CheckClipping(clipPlane, objectModel->GetObjectBoundingBox(), needsClipping))
      {
        continue;
      }
    }

    // process terrain mesh
    if (terrainSystem.IsValid())
    {
      // temporary buffers
      csArray<csVector3> cellVertices;
      csArray<csTriangle> cellTriangles;
      csVector3* polyVertices = new csVector3[4];

      size_t size = terrainSystem->GetCellCount();
      for (size_t j = 0; j < size; j++)
      {
        csRef<iTerrainCell> cell = terrainSystem->GetCell(j);

        // get cell data
        int width = cell->GetGridWidth();
        int height = cell->GetGridHeight();

        float offset_x = cell->GetPosition().x;
        float offset_y = cell->GetPosition().y;

        float scale_x = cell->GetSize().x / (width - 1);
        float scale_z = cell->GetSize().z / (height - 1);

        // tesselate and clip cell
        for(int y = 0; y < (height-2); ++y)
        {
          for(int x = 0; x < (width-2); ++x)
          {
            float xbase = x*scale_x+offset_x;
            float ybase = (height - 1 - y)*scale_z+offset_y;

            csVector3* polyVerts = polyVertices;
            polyVerts[0] = csVector3(xbase, cell->GetHeight(x,y), ybase);
            polyVerts[1] = csVector3(xbase+scale_x, cell->GetHeight(x+1,y), ybase);
            polyVerts[2] = csVector3(xbase+scale_x, cell->GetHeight(x+1,y+1), ybase-scale_z);
            polyVerts[3] = csVector3(xbase, cell->GetHeight(x,y+1), ybase-scale_z);

            SplitPolygon(vertices.GetSize(), 4, polyVerts, cellVertices, cellTriangles, transform, clipPlane, needsClipping);
          }
        }
      }

      vertices.Merge(cellVertices);
      triangles.Merge(cellTriangles);

      // free temporary buffers
      delete [] polyVertices;
    }
    // process trimesh
    else if (triangleMesh.IsValid()) 
    {
      int numberOfMeshTriangles = triangleMesh->GetTriangleCount();
      if (numberOfMeshTriangles > 0)
      {
        csArray<csVector3> meshVertices;
        csArray<csTriangle> meshTriangles;

        // Copy triangles
        csTriangle* triangleData = triangleMesh->GetTriangles();
        csVector3* vertexData = triangleMesh->GetVertices();

        csVector3* triVertices = new csVector3[3];

        for (int k = 0; k < numberOfMeshTriangles; k++)
        {
          // build triangle
          csVector3* triVerts = triVertices;
          triVerts[0] = vertexData[triangleData[k][0]];
          triVerts[1] = vertexData[triangleData[k][1]];
          triVerts[2] = vertexData[triangleData[k][2]];

          // clip triangle
          SplitPolygon(vertices.GetSize(), 3, triVerts, meshVertices, meshTriangles, transform, clipPlane, needsClipping);
        }

        delete [] triVertices;

        vertices.Merge(meshVertices);
        triangles.Merge(meshTriangles);
      }
    }
  }

  // allocate vertex buffer
  triangleVertices = new float[vertices.GetSize() * 3];
  if (!triangleVertices) 
  {
    return csApplicationFramework::ReportError("Out of memory while loading triangle data from sector.");
  }

  // copy vertex array data to the buffer
  numberOfVertices = vertices.GetSize();
  size_t i;
  for (i = 0; i < vertices.GetSize(); ++i)
  {
    const csVector3& v = vertices[i];
    triangleVertices[3*i  ] = v[0];
    triangleVertices[3*i+1] = v[1];
    triangleVertices[3*i+2] = v[2];
  }

  // allocate indice buffer
  triangleIndices = new int[triangles.GetSize() * 3];
  if (!triangleIndices) 
  {
    return csApplicationFramework::ReportError("Out of memory while loading triangle data from sector.");
  }

  // copy index array data to the buffer
  numberOfTriangles = triangles.GetSize();
  for (i = 0; i < triangles.GetSize(); ++i)
  {
    const csTriangle& t = triangles[i];
    triangleIndices[3*i  ] = t[0];
    triangleIndices[3*i+1] = t[1];
    triangleIndices[3*i+2] = t[2];
  }

  // Calculate a bounding box for the map triangles
  rcCalcBounds(triangleVertices, vertices.GetSize(), boundingMin, boundingMax);

  // free previously allocated trimesh
  delete chunkyTriMesh;

  // allocate new trimesh structure
  chunkyTriMesh = new rcChunkyTriMesh;
  if (!chunkyTriMesh) 
  {
    return csApplicationFramework::ReportError("Out of memory while loading triangle data from sector.");
  }

  // create new trimesh
  if (!rcCreateChunkyTriMesh(triangleVertices, triangleIndices, triangles.GetSize(), 256, chunkyTriMesh)) 
  {
    return csApplicationFramework::ReportError("Error creating ChunkyTriMesh.");
  }	

  return true;
}

// Based on Recast Sample_TileMesh::buildAllTiles()
THREADED_CALLABLE_IMPL(celNavMeshBuilder,BuildNavMesh)
{
  CS_ASSERT(currentSector);
  if (!currentSector) 
  {
    return false;
  }

  navMesh.AttachNew(new celNavMesh(objectRegistry));
  CS_ASSERT(navMesh.IsValid());
  navMesh->Initialize(parameters, currentSector, boundingMin, boundingMax);

  const float cellSize = parameters->GetCellSize();
  const int tileSize = parameters->GetTileSize();
  int gridWidth = 0, gridHeight = 0;
  rcCalcGridSize(boundingMin, boundingMax, cellSize, &gridWidth, &gridHeight);
  const int tw = (gridWidth + tileSize - 1) / tileSize;
  const int th = (gridHeight + tileSize - 1) / tileSize;
  const float tcs = tileSize * cellSize;

  rcConfig tileConfig;
  memset(&tileConfig, 0, sizeof(tileConfig));
  tileConfig.cs = cellSize;
  tileConfig.ch = parameters->GetCellHeight();  
  tileConfig.walkableHeight = (int)ceilf(parameters->GetAgentHeight() / tileConfig.ch);
  tileConfig.walkableRadius = (int)ceilf(parameters->GetAgentRadius() / cellSize);
  tileConfig.walkableClimb = (int)floorf(parameters->GetAgentMaxClimb() / tileConfig.ch);  
  tileConfig.walkableSlopeAngle = parameters->GetAgentMaxSlopeAngle();
  tileConfig.maxEdgeLen = (int)(parameters->GetMaxEdgeLength() / cellSize);
  tileConfig.maxSimplificationError = parameters->GetMaxSimplificationError();
  tileConfig.minRegionArea = (int)rcSqr(parameters->GetMinRegionArea());
  tileConfig.mergeRegionArea = (int)rcSqr(parameters->GetMergeRegionArea());
  tileConfig.maxVertsPerPoly = parameters->GetMaxVertsPerPoly();
  tileConfig.tileSize = tileSize;
  tileConfig.borderSize = tileConfig.walkableRadius + 3; // Reserve enough padding.
  tileConfig.width = tileConfig.tileSize + tileConfig.borderSize * 2;
  tileConfig.height = tileConfig.tileSize + tileConfig.borderSize * 2;
  tileConfig.detailSampleDist = parameters->GetDetailSampleDist() < 0.9f ? 0 : cellSize * 
                                parameters->GetDetailSampleDist();
  tileConfig.detailSampleMaxError = tileConfig.ch * parameters->GetDetailSampleMaxError();

  float tileBoundingMin[3];
  float tileBoundingMax[3];
  for (int y = 0; y < th; ++y)
  {
    int percent = ((float)y/th)*100;
    csPrintf("%d%%\n",percent); // Print progress %
    for (int x = 0; x < tw; ++x)
    {
      tileBoundingMin[0] = boundingMin[0] + x * tcs;
      tileBoundingMin[1] = boundingMin[1];
      tileBoundingMin[2] = boundingMin[2] + y * tcs;

      tileBoundingMax[0] = boundingMin[0] + (x + 1) * tcs;
      tileBoundingMax[1] = boundingMax[1];
      tileBoundingMax[2] = boundingMin[2] + (y + 1) * tcs;

      rcVcopy(tileConfig.bmin, tileBoundingMin);
      rcVcopy(tileConfig.bmax, tileBoundingMax);
      tileConfig.bmin[0] -= tileConfig.borderSize * tileConfig.cs;
      tileConfig.bmin[2] -= tileConfig.borderSize * tileConfig.cs;
      tileConfig.bmax[0] += tileConfig.borderSize * tileConfig.cs;
      tileConfig.bmax[2] += tileConfig.borderSize * tileConfig.cs;

      int dataSize = 0;
      unsigned char* data = BuildTile(x, y, tileBoundingMin, tileBoundingMax, tileConfig, dataSize);
      if (data)
      {
        if (!navMesh->AddTile(data, dataSize))
        {
          dtFree(data);
          csApplicationFramework::ReportWarning("could not add tile at location %d, %d in sector %s",
              x, y, currentSector->QueryObject()->GetName());
          continue;
        }
      }
    }
    csPrintf(CS_ANSI_CURSOR_UP(1)); // go back one line
    csPrintf(CS_ANSI_CLEAR_LINE); // clear line
  }
  ret->SetResult(csRef<iBase>(navMesh));
  return true;
}

void celNavMeshBuilder::CleanUpTileData()
{
  delete [] triangleAreas;
  triangleAreas = 0;
  rcFreeHeightField(solid);
  solid = 0;
  rcFreeCompactHeightfield(chf);
  chf = 0;
  rcFreeContourSet(cSet);
  cSet = 0;
  rcFreePolyMesh(pMesh);
  pMesh = 0;
  rcFreePolyMeshDetail(dMesh);
  dMesh = 0;
}

// Based on Recast Sample_TileMesh::buildTileMesh()
// NOTE I left the original Recast comments
unsigned char* celNavMeshBuilder::BuildTile(const int tx, const int ty, const float* bmin, const float* bmax, 
                                            const rcConfig& tileConfig, int& dataSize)
{

  if (!triangleVertices || !triangleIndices || !chunkyTriMesh)
  {
    csApplicationFramework::ReportError("Tried to build a navigation mesh without having set a sector first.");
    return 0;
  }

  // Make sure memory from last run is freed correctly (so there are no memory leaks if BuildTile crashes)
  CleanUpTileData();

  // Allocate voxel heighfield where we rasterize our input data to.
  solid = rcAllocHeightfield();
  if (!solid)
  {
    csApplicationFramework::ReportError("Out of memory building navigation mesh.");
    return 0;
  }
  if (!rcCreateHeightfield(&dummy, *solid, tileConfig.width, tileConfig.height, tileConfig.bmin, tileConfig.bmax, 
                           tileConfig.cs, tileConfig.ch))
  {
    csApplicationFramework::ReportError("Failed to create Heightfield");
    return 0;
  }

  // Allocate array that can hold triangle flags.
  // If you have multiple meshes you need to process, allocate
  // and array which can hold the max number of triangles you need to process.
  triangleAreas = new unsigned char[chunkyTriMesh->maxTrisPerChunk];
  if (!triangleAreas)
  {
    csApplicationFramework::ReportError("Out of memory building navigation mesh.");
    return 0;
  }

  float tbmin[2], tbmax[2];
  tbmin[0] = tileConfig.bmin[0];
  tbmin[1] = tileConfig.bmin[2];
  tbmax[0] = tileConfig.bmax[0];
  tbmax[1] = tileConfig.bmax[2];
  int cid[512];// TODO: Make grow when returning too many items.
  const int ncid = rcGetChunksOverlappingRect(chunkyTriMesh, tbmin, tbmax, cid, 512);
  if (!ncid)
  {
    return 0;
  }

  int tileTriangleCount = 0;
  for (int i = 0; i < ncid; ++i)
  {
    const rcChunkyTriMeshNode& node = chunkyTriMesh->nodes[cid[i]];
    const int* tris = &chunkyTriMesh->tris[node.i * 3];
    const int ntris = node.n;

    tileTriangleCount += ntris;

    memset(triangleAreas, 0, ntris * sizeof(unsigned char));
    rcMarkWalkableTriangles(&dummy, tileConfig.walkableSlopeAngle, triangleVertices, numberOfVertices, tris, 
                            ntris, triangleAreas);

    rcRasterizeTriangles(&dummy, triangleVertices, numberOfVertices, tris, triangleAreas, ntris, *solid, 
                         tileConfig.walkableClimb);
  }

  delete [] triangleAreas;
  triangleAreas = 0;

  // Once all geoemtry is rasterized, we do initial pass of filtering to
  // remove unwanted overhangs caused by the conservative rasterization
  // as well as filter spans where the character cannot possibly stand.
  rcFilterLowHangingWalkableObstacles(&dummy, tileConfig.walkableClimb, *solid);
  rcFilterLedgeSpans(&dummy, tileConfig.walkableHeight, tileConfig.walkableClimb, *solid);
  rcFilterWalkableLowHeightSpans(&dummy, tileConfig.walkableHeight, *solid);

  // Compact the heightfield so that it is faster to handle from now on.
  // This will result more cache coherent data as well as the neighbours
  // between walkable cells will be calculated.
  chf = rcAllocCompactHeightfield();
  if (!chf)
  {
    csApplicationFramework::ReportError("Out of memory building navigation mesh.");
    return 0;
  }
  if (!rcBuildCompactHeightfield(&dummy, tileConfig.walkableHeight, tileConfig.walkableClimb, *solid, *chf))
  {
    csApplicationFramework::ReportError("failed to build compact heightfield");
    return 0;
  }

  rcFreeHeightField(solid);
  solid = 0;

  // Erode the walkable area by agent radius.
  if (!rcErodeWalkableArea(&dummy, tileConfig.walkableRadius, *chf))
  {
    csApplicationFramework::ReportError("failed to errode walkable area");
    return 0;
  }

  // (Optional) Mark areas.
  for (int i  = 0; i < numberOfVolumes; ++i)
  {
    rcMarkConvexPolyArea(&dummy, volumes[i].verts, volumes[i].nverts, volumes[i].hmin, volumes[i].hmax, 
                         (unsigned char)volumes[i].area, *chf);
  }

  // Prepare for region partitioning, by calculating distance field along the walkable surface.
  if (!rcBuildDistanceField(&dummy, *chf))
  {
    csApplicationFramework::ReportError("failed to build distance field");
    return 0;
  }

  // Partition the walkable surface into simple regions without holes.
  if (!rcBuildRegions(&dummy, *chf, tileConfig.borderSize, tileConfig.minRegionArea, tileConfig.mergeRegionArea))
  {
    csApplicationFramework::ReportError("failed to build regions");
    return 0;
  }

  // remove border mapping as we don't want those to be removed
  /*for(int i = 0; i < chf->spanCount; i++)
  {
    chf->areas[i] &= ~RC_BORDER_REG;
  }*/

  // Create contours.
  cSet = rcAllocContourSet();
  if (!cSet)
  {
    csApplicationFramework::ReportError("Out of memory building navigation mesh.");
    return 0;
  }
  if (!rcBuildContours(&dummy, *chf, tileConfig.maxSimplificationError, tileConfig.maxEdgeLen, *cSet))
  {
    csApplicationFramework::ReportError("failed to build contours");
    return 0;
  }
  if (cSet->nconts == 0)
  {
    return 0;
  }

  // Build polygon navmesh from the contours.
  pMesh = rcAllocPolyMesh();
  if (!pMesh)
  {
    csApplicationFramework::ReportError("Out of memory building navigation mesh.");
    return 0;
  }
  if (!rcBuildPolyMesh(&dummy, *cSet, tileConfig.maxVertsPerPoly, *pMesh))
  {
    csApplicationFramework::ReportError("failed to build poly mesh");
    return 0;
  }

  // Build detail mesh.
  dMesh = rcAllocPolyMeshDetail();
  if (!dMesh)
  {
    csApplicationFramework::ReportError("Out of memory building navigation mesh.");
    return 0;
  }
  if (!rcBuildPolyMeshDetail(&dummy, *pMesh, *chf, tileConfig.detailSampleDist, tileConfig.detailSampleMaxError, *dMesh))
  {
    csApplicationFramework::ReportError("fail to build poly mesh detail");
    return 0;
  }

  rcFreeCompactHeightfield(chf);
  chf = 0;
  rcFreeContourSet(cSet);
  cSet = 0;

  unsigned char* navData = 0;
  int navDataSize = 0;
  if (tileConfig.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
  {
    if (pMesh->nverts >= 0xffff)
    {
      // The vertex indices are ushorts, and cannot point to more than 0xffff vertices.
      csApplicationFramework::ReportError("number of vertices overflowed");
      return 0;
    }

    // Update poly flags from areas.
    for (int i = 0; i < pMesh->npolys; ++i)
    {
      if (pMesh->areas[i] == RC_WALKABLE_AREA)
        pMesh->areas[i] = SAMPLE_POLYAREA_GROUND;

      if (pMesh->areas[i] == SAMPLE_POLYAREA_GROUND ||
        pMesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
        pMesh->areas[i] == SAMPLE_POLYAREA_ROAD)
      {
        pMesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
      }
      else if (pMesh->areas[i] == SAMPLE_POLYAREA_WATER)
      {
        pMesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
      }
      else if (pMesh->areas[i] == SAMPLE_POLYAREA_DOOR)
      {
        pMesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
      }
    }

    dtNavMeshCreateParams params;
    memset(&params, 0, sizeof(params));
    params.verts = pMesh->verts;
    params.vertCount = pMesh->nverts;
    params.polys = pMesh->polys;
    params.polyAreas = pMesh->areas;
    params.polyFlags = pMesh->flags;
    params.polyCount = pMesh->npolys;
    params.nvp = pMesh->nvp;
    params.detailMeshes = dMesh->meshes;
    params.detailVerts = dMesh->verts;
    params.detailVertsCount = dMesh->nverts;
    params.detailTris = dMesh->tris;
    params.detailTriCount = dMesh->ntris;
    params.offMeshConVerts = offMeshConVerts;
    params.offMeshConRad = offMeshConRads;
    params.offMeshConDir = offMeshConDirs;
    params.offMeshConAreas = offMeshConAreas;
    params.offMeshConFlags = offMeshConFlags;
    params.offMeshConCount = numberOfOffMeshCon;
    params.walkableHeight = parameters->GetAgentHeight();
    params.walkableRadius = parameters->GetAgentRadius();
    params.walkableClimb = parameters->GetAgentMaxClimb();
    params.tileX = tx;
    params.tileY = ty;
    rcVcopy(params.bmin, bmin);
    rcVcopy(params.bmax, bmax);
    params.cs = tileConfig.cs;
    params.ch = tileConfig.ch;
    params.buildBvTree = true;                
    params.tileLayer = 0;

    if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
    {
      csApplicationFramework::ReportError("failed to create nav mesh data");
      return 0;
    }
  }

  rcFreePolyMesh(pMesh);
  pMesh = 0;
  rcFreePolyMeshDetail(dMesh);
  dMesh = 0;

  dataSize = navDataSize;
  return navData;
}

iCelNavMesh* celNavMeshBuilder::LoadNavMesh (iFile* file)
{
  navMesh.AttachNew(new celNavMesh(objectRegistry));
  navMesh->LoadNavMesh(file);
  return navMesh;
}

bool celNavMeshBuilder::UpdateNavMesh (celNavMesh* navMesh, const csBox3& boundingBox)
{
  const float cellSize = parameters->GetCellSize();
  const int tileSize = parameters->GetTileSize();
  int gridWidth = 0, gridHeight = 0;
  rcCalcGridSize(boundingMin, boundingMax, cellSize, &gridWidth, &gridHeight);
  const int tw = (gridWidth + tileSize - 1) / tileSize;
  const int th = (gridHeight + tileSize - 1) / tileSize;
  const float tcs = tileSize * cellSize;

  csVector3 min = boundingBox.Min();
  csVector3 max = boundingBox.Max();

  // No intersection between object and navmesh
  if (min.y > boundingMax[1] || max.y < boundingMin[1] || min.x > boundingMax[0] || max.x < boundingMin[0] ||
      min.z > boundingMax[2] || max.z < boundingMin[2])
  {
    return true;
  }

  // Calculate which tiles intersect with the object
  unsigned xmin = (min.x - boundingMin[0]) / tcs;
  unsigned xmax = (max.x - boundingMin[0]) / tcs;
  unsigned zmin = (min.z - boundingMin[2]) / tcs;
  unsigned zmax = (max.z - boundingMin[2]) / tcs;

  // Adjust boundaries to be within the navmesh
  if (xmin < 0)
  {
    xmin = 0;
  }
  if (zmin < 0)
  {
    zmin = 0;
  }
  if (xmax > (unsigned)tw)
  {
    xmax = tw;
  }
  if (zmax > (unsigned)th)
  {
    zmax = th;
  }

  // Set tile parameters
  rcConfig tileConfig;
  memset(&tileConfig, 0, sizeof(tileConfig));
  tileConfig.cs = cellSize;
  tileConfig.ch = parameters->GetCellHeight();  
  tileConfig.walkableHeight = (int)ceilf(parameters->GetAgentHeight() / tileConfig.ch);
  tileConfig.walkableRadius = (int)ceilf(parameters->GetAgentRadius() / cellSize);
  tileConfig.walkableClimb = (int)floorf(parameters->GetAgentMaxClimb() / tileConfig.ch);  
  tileConfig.walkableSlopeAngle = parameters->GetAgentMaxSlopeAngle();
  tileConfig.maxEdgeLen = (int)(parameters->GetMaxEdgeLength() / cellSize);
  tileConfig.maxSimplificationError = parameters->GetMaxSimplificationError();
  tileConfig.minRegionArea = (int)rcSqr(parameters->GetMinRegionArea());
  tileConfig.mergeRegionArea = (int)rcSqr(parameters->GetMergeRegionArea());
  tileConfig.maxVertsPerPoly = parameters->GetMaxVertsPerPoly();
  tileConfig.tileSize = tileSize;
  tileConfig.borderSize = tileConfig.walkableRadius + 3; // Reserve enough padding.
  tileConfig.width = tileConfig.tileSize + tileConfig.borderSize * 2;
  tileConfig.height = tileConfig.tileSize + tileConfig.borderSize * 2;
  tileConfig.detailSampleDist = parameters->GetDetailSampleDist() < 0.9f ? 0 : cellSize * 
                                parameters->GetDetailSampleDist();
  tileConfig.detailSampleMaxError = tileConfig.ch * parameters->GetDetailSampleMaxError();

  // Update tiles
  float tileBoundingMin[3];
  float tileBoundingMax[3];
  tileBoundingMin[1] = boundingMin[1];
  tileBoundingMax[1] = boundingMax[1];
  for (unsigned y = zmin; y <= zmax; ++y)
  {
    for (unsigned x = xmin; x <= xmax; ++x)
    {
      tileBoundingMin[0] = boundingMin[0] + x * tcs;
      tileBoundingMin[2] = boundingMin[2] + y * tcs;

      tileBoundingMax[0] = boundingMin[0] + (x + 1) * tcs;
      tileBoundingMax[2] = boundingMin[2] + (y + 1) * tcs;

      rcVcopy(tileConfig.bmin, tileBoundingMin);
      rcVcopy(tileConfig.bmax, tileBoundingMax);
      tileConfig.bmin[0] -= tileConfig.borderSize * tileConfig.cs;
      tileConfig.bmin[2] -= tileConfig.borderSize * tileConfig.cs;
      tileConfig.bmax[0] += tileConfig.borderSize * tileConfig.cs;
      tileConfig.bmax[2] += tileConfig.borderSize * tileConfig.cs;

      int dataSize = 0;
      unsigned char* data = BuildTile(x, y, tileBoundingMin, tileBoundingMax, tileConfig, dataSize);
      if (data)
      {        
        if (!navMesh->RemoveTile(x, y) || !navMesh->AddTile(data, dataSize))
        {
          dtFree(data);
          return false;
        }
      }
    }
  }
  return true;
}

const iCelNavMeshParams* celNavMeshBuilder::GetNavMeshParams () const
{
  return parameters;
}

void celNavMeshBuilder::SetNavMeshParams (const iCelNavMeshParams* parameters)
{
  this->parameters.AttachNew(new celNavMeshParams(parameters));
  if(currentSector)
  {
    GetSectorData(); // recreate the sector data based on the new parameters
  }
}

iSector* celNavMeshBuilder::GetSector () const
{
  return currentSector;
}

}
CS_PLUGIN_NAMESPACE_END(celNavMesh)
