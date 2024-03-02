#include "stdafx.h"
#include "common.h"
#include "path_node.h"
#include "pathfinding.h"
#include "location.h"
#include "tio/tio.h"
#include "objlist.h"
#include "obj.h"
#include "gamesystems/map/sector.h"
#include "raycast.h"
#include "gameview.h"

//rendering
#include <tig\tig_startup.h>
#include <graphics/shaperenderer3d.h>
#include <graphics\textengine.h>
#include "ui/widgets/widget_styles.h"

PathNodeSys pathNodeSys;
char PathNodeSys::pathNodesLoadDir[260];
char PathNodeSys::pathNodesSaveDir[260];
MapPathNodeList * PathNodeSys::pathNodeList;
MapClearanceData PathNodeSys::clearanceData;
bool PathNodeSys::hasClearanceData = false;
struct PathNodeSysAddresses : temple::AddressTable
{
	
} addresses;


void PathNodeSys::RecipDebug()
{

	// check reciprocity
	struct RecipCheck
	{
		int nodeId;
		MapPathNodeList * node;
		int neighIds[MAX_NEIGHBOURS];
		MapPathNode neighbours[MAX_NEIGHBOURS];
		int nonrecipratingNodes[MAX_NEIGHBOURS];
		float distances[MAX_NEIGHBOURS];
		float distances2[MAX_NEIGHBOURS]; // for the opposite direction
		int numNonRecip;
		RecipCheck()
		{
			node = nullptr;
			nodeId = -1;
			numNonRecip = 0;
		}
	};

	auto oneSided = new RecipCheck[MaxPathNodes];
	int n = 0;

	auto node = pathNodeList;
	while (node)
	{
		bool hasNonRecip = false;
		for (int i = 0; i < node->node.neighboursCount; i++)
		{
			int naughtyNeighbour = 0;
			MapPathNode neighbour;
			GetPathNode(node->node.neighbours[i], &neighbour);
			bool isReciprocal = false;
			for (int j = 0; j < neighbour.neighboursCount && !isReciprocal; j++)
			{
				if (neighbour.neighbours[j] == node->node.id)
					isReciprocal = true;
			}
			if (!isReciprocal)
			{
				hasNonRecip = true;
				oneSided[n].nodeId = node->node.id;
				oneSided[n].node = node;
				oneSided[n].nonrecipratingNodes[oneSided[n].numNonRecip] = neighbour.id;
				oneSided[n].neighbours[oneSided[n].numNonRecip++] = neighbour;
			}
		}
		if (hasNonRecip)
		{
			n++;
		}
		node = node->next;
	}

	bool isFixable = true;
	if (n>0)
	{
		logger->info("There are {} nodes with non-reciprocating neighbours.", n);
	}
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < oneSided[i].numNonRecip; j++)
		{
			// first verify the direct path - this should be ok
			PathQuery pathQ;
			PathQueryResult path;
			pathQ.from = oneSided[i].node->node.nodeLoc;
			pathQ.to = oneSided[i].neighbours[j].nodeLoc;
			pathQ.flags = (PathQueryFlags)(PQF_DONT_USE_STRAIGHT_LINE | PQF_DONT_USE_PATHNODES | PQF_TO_EXACT);
			pathQ.critter = 0;
			pathQ.flags2 = 0;
			if (pathfindingSys.FindPath(&pathQ, &path) > 0)
			{
				auto pathLen = pathfindingSys.GetPathLength(&path);
				auto fromToDist = locSys.distBtwnLocAndOffs(pathQ.from, pathQ.to) / 12.0;
				oneSided[i].distances[j] = pathLen;
			} else
			{
				oneSided[i].distances[j] = -1.0;
			}

			// now try the opposite path
			pathQ.from = oneSided[i].neighbours[j].nodeLoc;
			pathQ.to = oneSided[i].node->node.nodeLoc;
			pathQ.flags = (PathQueryFlags)(PQF_DONT_USE_STRAIGHT_LINE | PQF_DONT_USE_PATHNODES | PQF_TO_EXACT);
			pathQ.critter = 0;
			pathQ.flags2 = 0;
			if (pathfindingSys.FindPath(&pathQ, &path) > 0)
			{
				auto pathLen2 = pathfindingSys.GetPathLength(&path);
				auto fromToDist = locSys.distBtwnLocAndOffs(pathQ.from, pathQ.to) / 12.0;
				oneSided[i].distances2[j] = pathLen2;
			}
			else // failed to path back; let's see if it's critters blocking
			{
				pathQ.from = oneSided[i].neighbours[j].nodeLoc;
				pathQ.to = oneSided[i].node->node.nodeLoc;
				pathQ.flags = (PathQueryFlags)(PQF_DONT_USE_STRAIGHT_LINE | PQF_DONT_USE_PATHNODES | PQF_TO_EXACT | PQF_IGNORE_CRITTERS | PQF_IGNORE_CRITTERS_ON_DESTINATION);
				pathQ.critter = 0;
				pathQ.flags2 = 0;
				if (pathfindingSys.FindPath(&pathQ, &path) > 0)
				{
					auto pathLen2 = pathfindingSys.GetPathLength(&path);
					oneSided[i].distances2[j] = pathLen2;
				} 
				else
				{
					isFixable = false;
					oneSided[i].distances2[j] = -1.0;
				}
					
			}

		}
	}

	if (!isFixable)
	{
		logger->info("Cannot reform all non-reciprocating path nodes!");
	} else if (n > 0)
	{
		logger->info("Can reform all {} nodes with non-reciprocating neighbours.",n);
	}
	free(oneSided);
	// hommlet results:
	// 90 cases of nodes with non-reciprocatig neighbours
	// all of them are actually pathable so long as you ignore critters on the destination tile
}

BOOL PathNodeSys::LoadNodeFromFile(TioFile* file, MapPathNodeList** nodeOut)
{
	int result = 0;

	if (!file)
		return 0;

	if (!nodeOut)
		return 0;

	MapPathNodeList * newNode = new MapPathNodeList;
	*nodeOut = newNode;

	if (!newNode)
		return 0;

	if (!tio_fread(&newNode->node.id, 4, 1, file)
		|| !tio_fread(&newNode->node.nodeLoc, sizeof(LocAndOffsets), 1, file)
		|| !tio_fread(&newNode->node.neighboursCount, 4, 1, file))
		return 0;
	
	auto neighCnt = newNode->node.neighboursCount;
	if (!neighCnt)
	{ // why are we holding nodes without neighbours? they should probably be culled...
		return 1;
	}
	if (neighCnt > MAX_NEIGHBOURS)
	{
		logger->info("Too many neighbours for node {}", newNode->node.id);
		assert(neighCnt <= MAX_NEIGHBOURS);
	}


	// newNode->node.neighbours = new int32_t[neighCnt];
	newNode->node.neighbours = new int32_t[MAX_NEIGHBOURS];
	if (tio_fread(newNode->node.neighbours, sizeof(int), neighCnt, file) != neighCnt)
		return 0;


	// newNode->node.neighDistances = new float[newNode->node.neighboursCount];
	newNode->node.neighDistances = new float[MAX_NEIGHBOURS];
	memset(newNode->node.neighDistances, 0, sizeof(float)* newNode->node.neighboursCount);
	
		return 1;
}

bool PathNodeSys::LoadNeighDistFromFile(TioFile* file, MapPathNodeList* node)
{
	
	int result = 0;

	if (!file)
		return false;

	if (!node)
		return false;



	auto neighCnt = node->node.neighboursCount;

	int nodeId;
	if (!tio_fread(&nodeId, 4, 1, file))
		return false;

	if (nodeId != node->node.id)
		return false;

	if (!tio_fread(&neighCnt, 4, 1, file))
		return false;

	if (neighCnt != node->node.neighboursCount)
		return false;

	if (!neighCnt)
	{
		return true;
}

	if (tio_fread(node->node.neighDistances, sizeof(float), neighCnt, file) == neighCnt)
	{
		*(int*)&node->node.flags |= PNF_NEIGHBOUR_DISTANCES_SET;
		return 1;
	}
	return false;

}

BOOL PathNodeSys::LoadNodesCurrent()
{
	//int orphanNodeCount = 0;
	//int orphanNodes[MaxPathNodes] ={};

	char fileName[260];
	char fileNameNew[260];
	char supplem[260];
	char clearanceFileName[260];

	_snprintf(fileName, 260, "%s\\%s", pathNodesLoadDir, "pathnode.pnd");
	_snprintf(fileNameNew, 260, "%s\\%s", pathNodesLoadDir, "pathnodenew.pnd");
	_snprintf(supplem, 260, "%s\\%s", pathNodesLoadDir, "pathnodedist.pnd");
	_snprintf(clearanceFileName, 260, "%s\\%s", pathNodesLoadDir, "clearance.bin");

	auto file = tio_fopen(fileNameNew, "rb");
	if (!file)
	{
		file = tio_fopen(fileName, "rb");
		if (!file)
			return 1;
	}

	auto clearanceFile = tio_fopen(clearanceFileName, "rb");
	if (hasClearanceData)
	{
		free(clearanceData.secClr);
	}
	hasClearanceData = false;
	if (clearanceFile)
	{
		int readStatus = tio_fread(&clearanceData.clrIdx, sizeof(clearanceData.clrIdx), 1, clearanceFile);
		if (readStatus)
		{	
			clearanceData.secClr = new SectorClearanceData[clearanceData.clrIdx.numSectors];
			readStatus = tio_fread(clearanceData.secClr, sizeof(SectorClearanceData), clearanceData.clrIdx.numSectors, clearanceFile);
		}
			
		tio_fclose(clearanceFile);
		if (readStatus)
			hasClearanceData = true;
	}


	auto fileSupplem = tio_fopen(supplem, "rb");
	int nodeCountSupplem = 0;
	if (fileSupplem)
	{
		if (!tio_fread(&nodeCountSupplem, 4, 1 ,fileSupplem))
			nodeCountSupplem = 0;
	}

	int nodeCount = 0;
	int status = 0;
	MapPathNodeList * newNode;
	if (tio_fread(&nodeCount, 4, 1, file) == 1)
	{
		status = 1;
		for (int i = 0; i < nodeCount; i++)
		{
			if (LoadNodeFromFile(file, &newNode ))
			{
				if (nodeCountSupplem == nodeCount)
					if (!LoadNeighDistFromFile(fileSupplem, newNode))
					{
						int dummy = 1;
					};
				newNode->next = pathNodeList;
				pathNodeList = newNode;
				/*
				if (newNode->node.neighboursCount == 0)
				{
					orphanNodes[orphanNodeCount++] = newNode->node.id;
				}
				*/
				if (pathNodeList->node.neighboursCount >0 &&  pathNodeList->node.neighDistances[0] < 0.1)
				{
					int dummy = 1;
				}
				else
				{
					int dum = 1;
				}
			} else
			{
				logger->warn("path_node.cpp: LoadNodesCurrent(): failed to read from file");
				status = 0;
			}
		}
	}
	tio_fclose(file);
	if (fileSupplem) tio_fclose(fileSupplem);
	return status;
}

void PathNodeSys::FreeNode(MapPathNodeList* pn)
{
	int neighCnt = pn->node.neighboursCount;

	if (neighCnt)
	{
		for (int i = 0; i < neighCnt; i++)
		{
			auto neighId = pn->node.neighbours[i];
			auto node = pathNodeList;
			while (node)
			{
				if (node->node.id == neighId)
					*(int*)&node->flags |= PNF_NEIGHBOUR_STATUS_CHANGED;
				node = node->next;
			}
		}
		free(pn->node.neighbours);
		//if (pn->node.flags & PNF_NEIGHBOUR_DISTANCES_SET)
			free(pn->node.neighDistances);
	}
	
	
	free( pn);
}

void PathNodeSys::PopNode(MapPathNodeList* node)
{
	
	if (!pathNodeList || !node)
		return;

	if (node == pathNodeList) {
		pathNodeList = node->next;
		return;
	}
	
	auto prev = pathNodeList;
	while ( prev->next && prev->next != node) {
		prev = prev->next;
	}
	prev->next = node->next;
	
}

BOOL PathNodeSys::FreeAndLoad(char* loadDir, char* saveDir)
{
	Reset();
	SetDirs(loadDir, saveDir);
	return LoadNodesCurrent();
}

void PathNodeSys::Reset()
{
	auto node = pathNodeList;
	while (node)
	{
		pathNodeList = node->next;
		FreeNode(node);
		node = pathNodeList;
	}
	mActivePathNode = nullptr;
	mPathnodeMoveRef = LocAndOffsets::null;
}

void PathNodeSys::SetDirs(char* loadDir, char* saveDir)
{
	strncpy(pathNodesLoadDir, loadDir, 260);
	strncpy(pathNodesSaveDir, saveDir, 260);
}

void PathNodeSys::RecalculateNeighbours(MapPathNodeList* node)
{
	if (!node)
		return;
	auto neighbor = pathNodeList;
	PathQuery pathQ;
	PathQueryResult path;
	while (neighbor)
	{
		if (node->node.id == neighbor->node.id)
		{
			neighbor = neighbor->next;
			continue;
		}
			
		pathQ.from = node->node.nodeLoc;
		pathQ.to = neighbor->node.nodeLoc;
		pathQ.flags = (PathQueryFlags) (PQF_DONT_USE_STRAIGHT_LINE | PQF_DONT_USE_PATHNODES | PQF_TO_EXACT | PQF_IGNORE_CRITTERS | PQF_IGNORE_CRITTERS_ON_DESTINATION);
		pathQ.critter = 0;
		pathQ.flags2 = 0;
		if (pathfindingSys.FindPath(&pathQ, &path ) > 0)
		{
			auto pathLen = pathfindingSys.GetPathLength(&path);
			auto fromToDist = locSys.distBtwnLocAndOffs(pathQ.from, pathQ.to) / 12.0;

			if ( pathLen < 6 * fromToDist && node->node.neighboursCount < MAX_NEIGHBOURS) // prevent extremely roundabout links
			{
				
				auto neighCntNew = ++node->node.neighboursCount;
				//int neighSizeNew = 4 * (node->node.neighboursCount + 1);
				// int *neighboursNew = (int*)realloc(neighbours, neighSizeNew);
				
				//node->node.neighbours = neighboursNew;
				//neighboursNew[neighCntNew - 1] = neighbor->node.id;
				node->node.neighbours[neighCntNew - 1] = neighbor->node.id;

				// node->node.neighDistances = (float*)realloc(node->node.neighDistances, neighSizeNew);
				if  (path.flags & PF_STRAIGHT_LINE_SUCCEEDED)
					node->node.neighDistances[neighCntNew - 1] = -pathLen;
				else
					node->node.neighDistances[neighCntNew - 1] = pathLen;
				

			}
			else
			{
				int aha = 0;
			}
		}
		neighbor = neighbor->next;
	}
	*(int*)&node->node.flags |= PNF_NEIGHBOUR_DISTANCES_SET;
}

void PathNodeSys::RecalculateAllNeighbours()
{
	auto node = pathNodeList;
	int maxNeighbours = 0;
	while(node)
	{
		int prevNeighCnt = node->node.neighboursCount;
		int prevNeighs[MAX_NEIGHBOURS] = {};
		memcpy(prevNeighs, node->node.neighbours, sizeof(int) * prevNeighCnt);
		if (node->node.neighboursCount > 0)
		{
			free(node->node.neighbours);
			if (node->node.flags & PNF_NEIGHBOUR_DISTANCES_SET)
				free(node->node.neighDistances);
			*(int*)&node->flags &= ~(PNF_NEIGHBOUR_STATUS_CHANGED );
			node->node.neighboursCount = 0;
			node->node.neighbours = nullptr;
			node->node.neighDistances = nullptr;
		}
		node->node.neighbours = new int[MAX_NEIGHBOURS];
		node->node.neighDistances = new float[MAX_NEIGHBOURS];
		RecalculateNeighbours(node);
		int newNeighCnt = node->node.neighboursCount;
		if (newNeighCnt > maxNeighbours)
			maxNeighbours = newNeighCnt;
		if (newNeighCnt != prevNeighCnt)
		{
			int newNeighs[MAX_NEIGHBOURS] = {};
			memcpy(newNeighs, node->node.neighbours, sizeof(int)*newNeighCnt);
			logger->info("new node connectivity for node {}: old was {} neighbours, new is {}", node->node.id ,prevNeighCnt, newNeighCnt);
		}
		node = node->next;
	}
	logger->info("Finished recalculating neighbours. Max neighbours is {}", maxNeighbours);
	// verify reciprocity
	node = pathNodeList;
	while( node)
	{
		for (int i = 0; i < node->node.neighboursCount;i++)
		{
			MapPathNode neighbour;
			GetPathNode(node->node.neighbours[i], &neighbour);
			bool isReciprocal = false;
			for (int j = 0; j < neighbour.neighboursCount && !isReciprocal;j++)
			{
				if (neighbour.neighbours[j] == node->node.id)
					isReciprocal = true;
			}
			if (!isReciprocal)
			{
				logger->info("Recalculate Neighbours - node {} has at least one shitty neighbour", node->node.id);
				int dummy = 1;
			}
		}
		node = node->next;
	}

	mNeedsRecalcNeighbours = false;
}



BOOL PathNodeSys::FlushNodes(const char* saveDir)
{
	int status = 0;
	int foundReleased = 0;
	int numNodes = 0;
	auto node = pathNodeList;

	while(node)
	{
		numNodes++;
		if (node->flags & PNF_NEIGHBOUR_STATUS_CHANGED)
			foundReleased = 1;
		node = node->next;
	}
	if (foundReleased)
		RecalculateAllNeighbours();

	char fileName[260];
	char fileNameSupplem[260];
	if (!saveDir || !saveDir[0]){
		saveDir = pathNodesSaveDir;
	}
	_snprintf(fileName, 260, "%s\\%s", saveDir, "pathnodenew.pnd");
	_snprintf(fileNameSupplem, 260, "%s\\%s", saveDir, "pathnodedist.pnd");
	auto file = tio_fopen(fileName, "wb");
	if (!file)
		return 0;

	auto fileSupplem = tio_fopen(fileNameSupplem, "wb");
	if (!fileSupplem)
		return 0;

	if (tio_fwrite(&numNodes,4,1,file) == 1 && tio_fwrite(&numNodes, 4,1,fileSupplem) == 1)
	{
		node = pathNodeList;
		status = 1;
		while (node)
		{
			if (!WriteNodeToFile(node, file))
			{
				logger->warn("path_node.cpp: FlushNodes() error writing to file");
				status = 0;
			}
			if (!WriteNodeDistToFile(node, fileSupplem))
			{
				logger->warn("path_node.cpp: FlushNodes() error writing neighbour distances to file");
				status = 0;
			}
			node = node->next;
		}
	}

	tio_fclose(file);
	tio_fclose(fileSupplem);
	return status;
}

void PathNodeSys::GenerateClearanceFile(const char * saveDir)
{	

	logger->info("Generating clearance data.");
	int idx = -1;
	auto secClrData = new SectorClearanceData();
	clearanceData.Reset();

	for (int secY = 0; secY < 16; secY++)
	{
		for (int secX = 0; secX < 16; secX++)
		{
			SectorLoc secLoc;
			secLoc.raw = secX + (secY << 26);
			Sector * sect;
			int lockStatus = 0;
			if (sectorSys.SectorFileExists(secLoc))
			{
				lockStatus = sectorSys.SectorLock(secLoc, &sect);
				logger->info("Locking Sector:  X {} Y {}, status: {}", secX, secY, lockStatus);
			}
			
			if (!lockStatus)
			{
				clearanceData.clrIdx.clrAddr[secY][secX] = -1;
				continue;
			}
			clearanceData.clrIdx.clrAddr[secY][secX] = ++idx;
			if (idx)
				secClrData = (SectorClearanceData*)realloc(secClrData, (idx + 1)*sizeof(SectorClearanceData));
			Subtile subtile;
			for (int ny = 0; ny < 64 * 3; ny++)
			{
				subtile.y = secY * 64 * 3 + ny;
				for (int nx = 0; nx < 64 * 3; nx++)
				{
					float clearancesInch = MAX_OBJ_RADIUS_SUBTILES * (INCH_PER_TILE / 3) ;
					
					subtile.x = secX*64*3 + nx;
					LocAndOffsets loc;
					locSys.SubtileToLocAndOff(subtile, &loc);
					auto flags = sect->GetTileFlags(&loc);
					if (flags)
					{
						//auto tileFlags = sectorSys.GetTileFlags(loc);
						TileFlags flagsToCheck = (TileFlags)((TileFlags::BlockX0Y0 + TileFlags::FlyOverX0Y0) << (nx % 3 + 3 * (ny % 3)));
						if (flagsToCheck & flags)
						{
							secClrData[idx].val[ny][nx] = 0;
							continue;
						}
					}

					RaycastPacket rayPkt;
					rayPkt.flags = (RaycastFlags)(RaycastFlags::HasToBeCleared | RaycastFlags::ExcludeItemObjects | RaycastFlags::ExcludePortals | RaycastFlags::HasRadius);
					rayPkt.radius = MAX_OBJ_RADIUS_TILES * INCH_PER_TILE;
					rayPkt.targetLoc = rayPkt.origin = loc;
					rayPkt.RaycastShortRange();
					int numFound = rayPkt.resultCount;
					for (int i = 0; i < numFound; i++)
					{
						auto res = &rayPkt.results[i];
						if (!res->obj)
						{

							float clrRad = locSys.Distance3d(loc, res->loc) ;
							if (clrRad < clearancesInch)
								clearancesInch = clrRad;
							if (clearancesInch == 0)
								break;

						}
					}
					secClrData[idx].val[ny][nx] = clearancesInch;
				}// nx
			} // ny
			sectorSys.SectorUnlock(secLoc);
			logger->info("Sector unlocked.");
		}
	}
	
	logger->info("Processing complete; saving to file clearance.bin");
	clearanceData.clrIdx.numSectors = idx + 1;
	clearanceData.secClr = secClrData;
	hasClearanceData = true;

	char fileName[260];
	if (!saveDir || !saveDir[0]) {
		saveDir = pathNodesSaveDir;
	}
	_snprintf(fileName, 260, "%s\\%s", saveDir, "clearance.bin");

	auto fil = tio_fopen(fileName, "wb" );
	tio_fwrite(&clearanceData.clrIdx, sizeof(clearanceData.clrIdx), 1, fil);
	tio_fwrite(clearanceData.secClr, sizeof(SectorClearanceData), clearanceData.clrIdx.numSectors, fil);
	logger->info("Wrote to file. Closing.");
	tio_fclose(fil);
}

int PathNodeSys::CalcClearanceFromNearbyObjects(objHndl obj, float clearanceReq)
{
	//memset(clearanceData, 255, sizeof(clearanceData));
	ObjList objList;
	LocAndOffsets objLoc = objects.GetLocationFull(obj);
	objList.ListRadius(objLoc, INCH_PER_TILE * 64, OLC_ALL & ~(OLC_ITEMS | OLC_TRAP ));
	uint8_t objRadiusInSubtiles;
	if (clearanceReq > INCH_PER_TILE * MAX_OBJ_RADIUS_SUBTILES)
		objRadiusInSubtiles = 255;
	else
		objRadiusInSubtiles = static_cast<unsigned char>(pow( (clearanceReq / (INCH_PER_TILE/3)) ,2)) ;
	int objListSize = objList.size();
	for (int i = 0; i < objListSize; i++)
	{
		objHndl objIter = objList.get(i);
		float objIterRadius = objects.GetRadius(objIter);
		char objIterRadiusInSubtiles = static_cast<unsigned char>(pow((objIterRadius / (INCH_PER_TILE / 3)), 2));
		LocAndOffsets objIterLoc = objects.GetLocationFull(objIter);
	}
	return -1;
}

BOOL PathNodeSys::WriteNodeToFile(MapPathNodeList* node, TioFile* file)
{
	BOOL result = 0;
	if (!file)
		return 0;
	if (!node)
		return 0;
	if (!tio_fwrite(&node->node.id, 4, 1, file) == 1)
		return 0;
	if (!tio_fwrite(&node->node.nodeLoc, sizeof(LocAndOffsets), 1, file))
		return 0;
	int  count = node->node.neighboursCount;
	if (tio_fwrite(&node->node.neighboursCount, 4, 1, file) == 1
		&& (!count || tio_fwrite(node->node.neighbours, sizeof(int), count, file) == count))
	{
		return 1;
	}
	return result;
}


void PathNodeSys::apply()
{
	
	//rebase(GetPathNode, 0x100A9660);
	//rebase(FindClosestPathNode, 0x100A96A0);
	//_FindPathBetweenNodes = temple::GetPointer<>(0x100A9E30);
	//pathNodeList = temple::GetPointer<MapPathNodeList*>(0x10BA62B0);
	replaceFunction(0x100A9720, SetDirs);
	replaceFunction(0x100A9C00, LoadNodesCurrent);
	replaceFunction<void(__cdecl)()>(0x100A9DA0, []() {
		pathNodeSys.Reset();
		});
	replaceFunction<BOOL(__cdecl)(char*,char*)>(0x100A9DD0, [](char* loadDir, char*saveDir) {
		return pathNodeSys.FreeAndLoad(loadDir, saveDir);
		});
	hasClearanceData = false;
	
	
	auto pathnodeListAddr = &pathNodeList;
	write(0x100A9460+1, &pathnodeListAddr, 4);
}

MapPathNodeList* PathNodeSys::GetPathNodeListEntry(int id)
{
	auto node = pathNodeList;
	while (node && node->node.id != id){
		node = node->next;
	}
	return node;
}

bool PathNodeSys::WriteNodeDistToFile(MapPathNodeList* node, TioFile* file)
{
	if (!file)
		return 0;
	if (!node)
		return 0;
	if (!tio_fwrite(&node->node.id, 4, 1, file) == 1)
		return 0;

	int count = node->node.neighboursCount;
	if (tio_fwrite(&count, 4, 1, file) == 1
		&& (!count || tio_fwrite(node->node.neighDistances, sizeof(float), count, file) == count)
		)
	{
		return true;
	}
	return false;
}

void PathNodeSys::FindPathNodeAppend(const FindPathNodeData& node) {
	fpbnData[fpbnCount++] = node;
}

int PathNodeSys::PopMinHeuristicNode(FindPathNodeData* fpndOut, bool useActualDist)
{

	if (!useActualDist)
		return PopMinHeuristicNodeLegacy(fpndOut);

	int idxMin;
	// find a node with a positive actual distance
	for (idxMin = 0; idxMin < fpbnCount; idxMin++)
		if (fpbnData[idxMin].distActualTotal >= 0.0)
			break;
	if (idxMin == fpbnCount)
		return 0;

	// search for minimum cumulative distance
	float minDistCumul = fpbnData[idxMin].distActualTotal + fpbnData[idxMin].distTo;
	for (int i = idxMin + 1; i < fpbnCount; i++)
	{
		if (fpbnData[i].distActualTotal >= 0.0
			&& ( fpbnData[i].distActualTotal + fpbnData[i].distTo ) < minDistCumul)
{
			idxMin = i;
			minDistCumul = fpbnData[idxMin].distActualTotal + fpbnData[idxMin].distTo;
		}
	}

	// copy it out
	*fpndOut = fpbnData[idxMin];

	// pop the found entry
	fpbnData[idxMin] = fpbnData[fpbnCount - 1];
	fpbnCount--;
	return 1;
}

int PathNodeSys::PopMinHeuristicNodeLegacy(FindPathNodeData* fpndOut)
{
	int idxMinCumul;
	// find a node with a positive cumulative distance
	for (idxMinCumul = 0; idxMinCumul < fpbnCount; idxMinCumul++)
		if (fpbnData[idxMinCumul].distCumul >= 0.0)
			break;
	if (idxMinCumul == fpbnCount)
		return 0;

	// search for minimum cumulative distance
	float minDistCumul = fpbnData[idxMinCumul].distCumul;
	for (int i = idxMinCumul + 1; i < fpbnCount; i++)
	{
		if (fpbnData[i].distCumul >= 0.0 
			&& fpbnData[i].distCumul < minDistCumul)
		{
			idxMinCumul = i;
			minDistCumul = fpbnData[idxMinCumul].distCumul;
		}
	}

	// copy it out
	*fpndOut = fpbnData[idxMinCumul];

	// pop the found entry
	fpbnData[idxMinCumul] = fpbnData[fpbnCount - 1];
	fpbnCount--;
	return 1;
}

BOOL PathNodeSys::FindClosestPathNode(LocAndOffsets* loc, int* nodeIdOut)
{
	float closestDist = 100000.0;
	float dist;
	auto node = pathNodeList;
	MapPathNodeList * closestNode = nullptr;

	while (node)
	{
		dist = locSys.distBtwnLocAndOffs(node->node.nodeLoc, *loc);
		if (!closestNode || dist < closestDist)
		{
			closestNode = node;
			closestDist = dist;
		}
		node = node->next;
	}

	if (closestNode )
	{
		if (nodeIdOut)
			*nodeIdOut = closestNode->node.id;
		return 1;
	}

	return 0;
}

int PathNodeSys::FindPathBetweenNodes(int fromNodeId, int toNodeId, int* nodeIds, int maxChainLength)
{
	MapPathNode fromNode, toNode;
	int chainLength = 0;
	int numNodes ;
	auto pnIterator = pathNodeList;
	for (numNodes = 0; pnIterator; ++numNodes)
		pnIterator = pnIterator->next;

	fpbnCount = 0;

	// find the from/to nodes
	pnIterator = pathNodeList;
	if (!pathNodeSys.GetPathNode(fromNodeId, &fromNode))
		return 0;
	if (!pathNodeSys.GetPathNode(toNodeId, &toNode))
		return 0;
	


	// determine if the pathnodes are using the supplemental information of Actual Travel Distance (NEW! TemplePlus only)
	// if so, the node distance evaluation algorithm will be quite different, taking into acconut the actual travel distance rather than the strange method employed by troika
	bool useActualDistances = false;
	if ((toNode.flags & PNF_NEIGHBOUR_DISTANCES_SET) && (fromNode.flags & PNF_NEIGHBOUR_DISTANCES_SET))
		useActualDistances = true;
	

	// testing neighbour IDs
	//int neighboursTest[MAX_NEIGHBOURS];
	//memcpy(neighboursTest, toNode.neighbours, toNode.neighboursCount * sizeof(int));
	


	// begin the A* algorithm
	float distFromTo = locSys.distBtwnLocAndOffs(fromNode.nodeLoc, toNode.nodeLoc) / 12.0f;
	
	FindPathNodeData fpMinCumul;
	fpMinCumul.nodeId = fromNodeId;
	fpMinCumul.refererId = -1;
	fpMinCumul.distFrom = 0;
	fpMinCumul.distTo = distFromTo;
	fpMinCumul.distCumul = distFromTo;
	fpMinCumul.distActualTotal = 0;
	fpMinCumul.heuristic = 0;
	fpMinCumul.usingActualDistance = useActualDistances;

	FindPathNodeAppend(fpMinCumul);

	if (!PopMinHeuristicNode(&fpMinCumul, useActualDistances))
	{
	//	delete fpbnData;
		return 0;
	}

	MapPathNode minCumulNode, neighNode;
	FindPathNodeData fpTemp;

	while(fpMinCumul.nodeId != toNodeId)
	{
		// find the matching Path Node
		pathNodeSys.GetPathNode(fpMinCumul.nodeId, &minCumulNode);

		// loop thru its neighbours, searching 
		for (int i = 0; i < minCumulNode.neighboursCount; i++)
		{
			fpTemp.refererId = fpMinCumul.nodeId;
			fpTemp.nodeId = minCumulNode.neighbours[i];
			
			// find the neighbour node
			int neighbourId = minCumulNode.neighbours[i];
			pathNodeSys.GetPathNode(neighbourId, &neighNode);

			// calculate its heuristic
			fpTemp.distTo = locSys.distBtwnLocAndOffs(neighNode.nodeLoc, toNode.nodeLoc) / 12.0f;
			fpTemp.distFrom = locSys.distBtwnLocAndOffs(fromNode.nodeLoc, neighNode.nodeLoc) / 12.0f;
			fpTemp.distCumul = fpMinCumul.distCumul + fpTemp.distTo + fpTemp.distFrom;
			if (useActualDistances)
			{
				fpTemp.distActualTotal = fpMinCumul.distActualTotal + minCumulNode.neighDistances[i];
				fpTemp.heuristic = fpTemp.distActualTotal + fpTemp.distTo;
			}

			bool foundNode = false;			
			for (auto j = 0; j < fpbnCount; ++j) {
				auto &node = fpbnData[j];
				if (node.nodeId != fpTemp.nodeId) {
					continue;
				}

				foundNode = true;

				// Effectively the same path segment (TODO: Is getting here a bug already? Is it searching in cycles?)
				if (node.refererId == fpTemp.refererId || true) {
					if (useActualDistances) {
						if (fpTemp.distActualTotal + fpTemp.distTo < node.distActualTotal + node.distTo) {
							node = fpTemp;
							break;
						}
					}
					else {
						if (fpTemp.distCumul < node.distCumul) {
							node = fpTemp;
							break;
						}
					}
				}
				/*
				if (useActualDistances) {
					if (fpTemp.distActualTotal + fpTemp.distTo < node.distActualTotal + node.distTo) {
						FindPathNodeAppend(fpTemp);
						break;
					}
				} else {
					if (fpTemp.distCumul < node.distCumul) {
						FindPathNodeAppend(fpTemp);
						break;
					}
				}
				*/
			}

			// append node if it's not in the list yet
			if (!foundNode) {
				FindPathNodeAppend(fpTemp);
			}

		}

		fpMinCumul.distCumul = -1.0;
		fpMinCumul.distActualTotal = -1.0;
		FindPathNodeAppend(fpMinCumul);

		if (!PopMinHeuristicNode(&fpMinCumul, useActualDistances))
		{
		//	delete fpbnData;
			return 0;
		}
	}

	fpTemp = fpMinCumul;

	int refId0 = fpMinCumul.refererId; // the node leading up to the last node
	chainLength = 1;

	// get the chain length
	int refererId = fpTemp.refererId;
	while (refererId != -1) {
		int refererFound = 0;
		for (int i = 0; i < fpbnCount; i++) {
			if (fpbnData[i].nodeId == refererId) {
				fpTemp = fpbnData[i];
				refererFound = 1;
				break;
			}
				
		}
		if (!refererFound) {
			break;
			//shit
		}
		chainLength++;
		refererId = fpTemp.refererId;
	}

	if (chainLength >= maxChainLength)
	{
	//	delete fpbnData;
		return 0;
		
	}

	// dump node chain into nodeIds
	fpTemp = fpMinCumul;
	refererId = fpMinCumul.refererId;
	for (int i = chainLength -1 ; i >= 0 ; i--)
	{
		nodeIds[i] = fpTemp.nodeId;
		int refererFound = 0;
		for (int refIdx = 0; refIdx < fpbnCount; refIdx++)
		{
			if (fpbnData[refIdx].nodeId == refererId)
			{
				fpTemp = fpbnData[refIdx];
				refererId = fpTemp.refererId;
				refererFound = 1;
				break;
			}

		}
		if (i == 0 && refererId != -1)
		{
			int breakPointDummy = 1;
		}

	}

//	delete fpbnData;
	return chainLength;
	


	// return _FindPathBetweenNodes(fromNodeId, toNodeId, nodeIds, maxChainLength);
}

void PathNodeSys::AddPathNode(LocAndOffsets& loc, bool recalcNeighbours)
{
	mNeedsRecalcNeighbours = true;

	MapPathNodeList* newNode = new MapPathNodeList();

	if (!newNode) return;

	newNode->node.id = GetNewId();
	newNode->node.nodeLoc = loc;
	

	newNode->node.neighbours = new int32_t[MAX_NEIGHBOURS];
	newNode->node.neighDistances = new float[MAX_NEIGHBOURS];
	
	
	if (recalcNeighbours) {
		RecalculateNeighbours(newNode);
		for (auto i = 0; i < newNode->node.neighboursCount; ++i) {
			auto neighbour = GetPathNodeListEntry(newNode->node.neighbours[i]);
			if (neighbour) {
				RecalculateNeighbours(neighbour);
			}
		}
	}

	// prepend to list (should not affect neighbour search since it does SANS_PATHNODE pathfinding)
	newNode->next = pathNodeList;
	pathNodeList = newNode;
	
	mNeedsRecalcNeighbours = true;
}

BOOL PathNodeSys::GetPathNode(int id, MapPathNode* pathNodeOut)
{
	auto node = GetPathNodeListEntry(id);
	if (!node)
		return 0;
	*pathNodeOut = node->node;
	return 1;
}

bool PathNodeSys::PathNodeExists(int id)
{
	auto node = pathNodeList;
	if (node)
	{
		while (node && node->node.id != id)
		{
			node = node->next;
		}
		if (!node)
			return false;
		return true;
	}
	return false;
}


void PathNodeSys::SetPathNodeVisibile(bool setting)
{
	mRenderPathNodesEn = setting;
}

/* replaces 0x100A9390 (not a 1:1 recreation however) */
void PathNodeSys::RenderPathNodes(int tileX1, int tileX2, int tileY1, int tileY2)
{
	if (!mRenderPathNodesEn)
		return;

	auto node = pathNodeList;
	
	auto& renderer3d = tig->GetShapeRenderer3d();
	XMCOLOR nodeBorderColor(0.95f, 0.95f, 0.05f, 1.0f);
	XMCOLOR aoeBorderColor(0.1f, 0.9f, 0.1f, 1.0f);

	XMCOLOR activeNodeBorderColor(0.95f, 0.95f, 0.95f, 1.0f);
	XMCOLOR activeAoeBorderColor(0.95f, 0.9f, 0.95f, 1.0f);

	XMCOLOR circleFillColor(0.0f, 0.0f, 0.0f, 0.0f);

	static gfx::TextStyle textStyle = widgetTextStyles->GetTextStyle("default");
	auto& textEngine = tig->GetRenderingDevice().GetTextEngine();

	for (auto node = pathNodeList; node; node = node->next) {
		auto nodeLoc = node->node.nodeLoc;
		
		if (nodeLoc.location.locx < static_cast<uint32_t>(tileX1) || nodeLoc.location.locx > static_cast<uint32_t>(tileX2) ||
			nodeLoc.location.locy < static_cast<uint32_t>(tileY1) || nodeLoc.location.locy > static_cast<uint32_t>(tileY2))
			continue;

		auto pos = nodeLoc.ToInches3D();


		if (node == mActivePathNode) {

			auto topOfNode = gameView->WorldToScreenUi(pos);

			renderer3d.DrawFilledCircle(pos, 8, activeNodeBorderColor, circleFillColor, false);
			renderer3d.DrawFilledCircle(pos, 603, activeAoeBorderColor, circleFillColor, false);

			gfx::FormattedText t;
			t.defaultStyle = textStyle;
			t.defaultStyle.align = gfx::TextAlign::Center;
			
			auto text = fmt::format("Node {}, neighbs:\n", node->node.id);
			for (auto i = 0; i < node->node.neighboursCount; ++i) {
				text += fmt::format("{}{} ({:.2f})", i==0 ? "" : ", ", node->node.neighbours[i], node->node.neighDistances[i]);
			}
			t.text = local_to_ucs2(text);
			auto& nameStyle = t.formats.push_back();
			nameStyle.startChar = 0;
			nameStyle.length = 5;
			nameStyle.style = t.defaultStyle;

			gfx::TextMetrics nameMetrics;
			nameMetrics.width = 120;
			textEngine.MeasureText(t, nameMetrics);

			TigRect nameRect;
			nameRect.x = static_cast<int>(topOfNode.x)- nameMetrics.width/2;
			nameRect.width = nameMetrics.width;
			nameRect.y = static_cast<int>(topOfNode.y) - nameMetrics.height - 2;
			nameRect.height = nameMetrics.height;
			textEngine.RenderText(nameRect, t);
		}


		renderer3d.DrawFilledCircle(pos, 6, nodeBorderColor, circleFillColor, false);
		renderer3d.DrawFilledCircle(pos, 600, aoeBorderColor, circleFillColor, false);

	}
}

bool PathNodeSys::SetActiveNodeFromLoc(LocAndOffsets& loc)
{
	mActivePathNode = nullptr;
	
	int nodeId = -1;
	if (!pathNodeSys.FindClosestPathNode(&loc, &nodeId)) {
		return false;
	}
	
	constexpr float DIST_THRESH = 2 * INCH_PER_FEET;
	auto node = GetPathNodeListEntry(nodeId);
	if (locSys.Distance3d(node->node.nodeLoc, loc) < DIST_THRESH) {
		mActivePathNode = node;
		return true;
	}
	return false;
}

void PathNodeSys::DeleteActiveNode(){
	if (!mActivePathNode)
		return;

	PopNode(mActivePathNode);
	FreeNode(mActivePathNode);
	mActivePathNode = nullptr;
}

bool PathNodeSys::MoveActiveNode(LocAndOffsets* newLoc)
{
	
	if (newLoc) { // comes from move handler, only if in PathNodeMove mode; return value says if it's ok to set new active node
		if (!mActivePathNode) {
			return true;
		}

		auto pickedNodeToMove = mPathnodeMoveRef != LocAndOffsets::null;
		if (pickedNodeToMove) {
			mActivePathNode->node.nodeLoc = *newLoc;
			return false;
		}
		else {
			return true;
		}
		
	}
	else { // comes from click handler
		if (!mActivePathNode)
			return false;
		if (mPathnodeMoveRef == LocAndOffsets::null) { // first click - node picked, get ref and start moving
			mPathnodeMoveRef = mActivePathNode->node.nodeLoc;
			return false;
		}
		else { // already got ref, so time to finish moving
			mPathnodeMoveRef = LocAndOffsets::null;
			return true;
		}
	}
	
	return false;
}

void PathNodeSys::CancelMoveActiveNode(){
	if (!mActivePathNode)
		return;
	if (mPathnodeMoveRef == LocAndOffsets::null) {
		return;
	}
	mActivePathNode->node.nodeLoc = mPathnodeMoveRef;
	mPathnodeMoveRef = LocAndOffsets::null;
}

int PathNodeSys::GetNewId(){
	auto node = pathNodeList;
	if (!node)
		return 0;

	int nextId = 0;
	const int MAX_ID = 1<<20;
	for (int i = 0; PathNodeExists(i) && i < MAX_ID; i++) {
		nextId++;
	}

	return nextId++;
}

MapPathNodeList::MapPathNodeList()
{
	memset(this, 0, sizeof(MapPathNodeList));
}

void MapClearanceData::Reset()
{
	if (this->secClr) {
		free(secClr);
		secClr = nullptr;
	}
	clrIdx.Reset();
}

MapClearanceData::MapClearanceData()
{
	secClr = nullptr;
	Reset();
}

ClearanceIndex::ClearanceIndex()
{
	Reset();
}

void ClearanceIndex::Reset()
{
	numSectors = 0;
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			clrAddr[i][j] = -1;
		}
	}
}
