// ITEMS.CPP
// Item-related code for BAM
#include "apires.hpp"
#include "tigre.hpp"
#include "list.hpp"
#include "periodic.hpp"
#include "savebase.hpp"
#include "maps.hpp"
#include "viewport.hpp"
#include "items.hpp"
#include "jstream.hpp"
#include "world.hpp"

char initItemNames[MAXITEMTYPES + 1][30] =
{"", "BERSERKER BREW", "BOAR BURGER", "BOG BOOTS", "DAEMON'S BANE", "DOWRY CHEST", "DRIFT DISC", "FROST CAPE",
 "HEALING SALVE", "KEY", "MAP", "MIGHT MANTLE", "SACRED URN", "COMPONENT", "SHRUB SPRITE", "STORM BRACERS",
 "TELEPORT TOME", "TRAILFINDER", "VERDANT SHIELD", "VIRTUE VEIL", "WEIRD WAND", "MASON MIX", "MANA ORB",
 "HARPY NET"};
extern World	*pWorld;

ItemMgr::ItemMgr(void) : lItems(30, true)
{
	Init();
}

bool
ItemMgr::Save(uint16 state, nlohmann::json& j)
{
	JsonStream js{j, (state == DURING_SAVE)};
	js.syncArray(fItemCanAppear, "fItemCanAppear");
	js.syncArray(maxItemCounts,  "maxItemCounts");
	return false;
}

void
ItemMgr::Cleanup(void)
{
//	int	loop1;
	grip	gItem;

	// clear out list of item locations
	gItem = ptr_to_grip(lItems.FirstValue());
	while(gItem)
	{
		ADelete(gItem);
		gItem = ptr_to_grip(lItems.NextValue());
	}
	lItems.Release();
}

ItemMgr::~ItemMgr()
{
}

int defaultMaxItemCounts[ENDOFITEMTYPES] =
{0, 5, 999, 3, 3, 4, 3, 3,
5, 4, 10, 3, 3, 6, 3, 3,
3, 3, 3, 3, 3, 5, 5,
3};
void
ItemMgr::Init(void)
{
	uint32 loop1;

	memset(itemCounts, 0, sizeof(itemCounts));
	memset(gItemMap, 0, sizeof(gItemMap));
	memset(attackMods, 0, sizeof(attackMods[0]) * MAXITEMTYPES);
	memset(attackRangeMods, 0, sizeof(attackRangeMods[0]) * MAXITEMTYPES);
	memset(defenseMods, 0, sizeof(defenseMods[0]) * MAXITEMTYPES);
	memset(moveRateMods, 0, sizeof(moveRateMods[0]) * MAXITEMTYPES);
	aniNums[0] = 0;

	memcpy(itemNames, initItemNames, sizeof(initItemNames));
	for(loop1 = 0; loop1 < MAXITEMTYPES; loop1++)
	{
		aniNums[loop1 + 1] = 2500 + (loop1 * 2) % 50 + loop1 / 25;	// compute ani # based on type #
		maxItemCounts[loop1 + 1] = defaultMaxItemCounts[loop1 + 1];
	}
	memset(fItemCanAppear, false, sizeof(fItemCanAppear));
}

// returns Ani # for given itemType
uint32
ItemMgr::GetPortAnim(itemType item)
{
	if(item == NO_ITEM)
		return(0);
	return(aniNums[item]);
}

// check for type of item (if any) at X,Y but do not remove from itemMap
BAMItem *
ItemMgr::CheckItem(uint32 tileX, uint32 tileY)
{
	BAMItem	*pItem;
	grip	gItem;
	
	gItem = gItemMap[tileY][tileX];
	if(!gItem)
		return(0);
	pItem = ADerefAs(BAMItem, gItem);
	return(pItem);
}

itemType
ItemMgr::GetItemType(char *szItem)
{
	int	loop1;

	for(loop1 = 0; loop1 <= MAXITEMTYPES; loop1++)
	{
		if(!strcmp(szItem, &itemNames[loop1][0]))
			return((itemType)loop1);
	}
	return(NO_ITEM);
}

itemType
ItemMgr::PickRandomItem(void)
{
	uint32 loop1, itemNum;

	// pick random slot in item list, and search forward til an avail item is found
	itemNum = ARandom(MAXITEMTYPES) + 1;
	for(loop1 = 0; loop1 < MAXITEMTYPES; loop1++)
	{
		if(fItemCanAppear[itemNum])
			return((itemType)itemNum);
		itemNum = (itemNum + 1) % MAXITEMTYPES + 1;
	}
	return(NO_ITEM);
}

// Alloc an item and place it in the world.
bool
ItemMgr::PlaceItem(uint32 tileX, uint32 tileY, itemType newItemType, int dropRadius, bool fSkipCenter)
{
	int		loop1, loopX, loopY;
	MapSpace	*pSpace;
	Terrain	*pTerrain;
	grip		gAni;
	tile		*pTile;
	Unit		*pUnit;
	bool		fDone;

	if(newItemType > MAXITEMTYPES || tileX >= WORLD_WIDTH || tileY >= WORLD_HEIGHT)
	{
		pMono->Out("ItemMgr::PlaceItem(X%d, Y%d, I%d) - error, invalid parameter.\n", tileX, tileY, newItemType);
		return(false);
	}

	fDone = false;
	for(loop1 = 0; loop1 <= dropRadius && !fDone; loop1++)
	{
		pWorld->map.GetSurroundingCoords(tileX, tileY, loop1, loopX, loopY, true);
		do
		{
			// if location already occupied by an item
			if(CheckItem(loopX, loopY) != nullptr && newItemType != NO_ITEM)
				continue;

			// is terrain unwalkable?
			pTerrain = pWorld->map.GetTerrain(loopX, loopY);
			if(!pTerrain || pTerrain->moveRate > 5)
				continue;

			// cant drop item on building
			pSpace = pWorld->map.GetStructure(loopX, loopY);
			if(pSpace && pSpace->h.currHP > 0)
				continue;

			// don't allow items to be lost under buildings
			pUnit = pWorld->unitLib.GetUnit(loopX, loopY);
			if(pUnit && pUnit->type == ACOLYTE && pUnit->currAction ==
				TRANSFORM_TO_BLDG)
				continue;

			if(fSkipCenter && loopX == tileX && loopY == tileY)
				continue;

			pTile = pWorld->map.GetTile(loopX, loopY);
			if(!pTile || pTile->terrain == CAULDRON)
				continue;

			// create and add an entry to the ItemList
			if(newItemType == COMPONENT)
			{
				// pTile already fetched above
				if(pTile->terrain == PORTAL)
				{
					pSpace = pWorld->map.GetStructure(loopX, loopY);
					if(pSpace && pSpace->h.owner == pWorld->playerSide)
					{
						gAni = pWorld->vPort.NewAni(RES_ANIM, ANIM_SPARKY_POOF, 1,
							PRI_ITEM, true, false, 4, loopX, loopY, false);
						pWorld->vPort.CheckAni(gAni);
						pWorld->componentsCaptured++;

						pWorld->Announce(tileX, tileY, SQUIB_WORLD_MSGS, (int)MSG_COMPONENT_CAPTURED);
//						pString = pWorld->squib1.Load(SQUIB_WORLD_MSGS, (int)MSG_COMPONENT_CAPTURED);
//						pWorld->MsgBoxAdd(pString, tileX, tileY);
//						pWorld->MsgBoxDraw();

						pWorld->CheckWinLose(tileX, tileY);

						return(true);
					}
				}
			}
			if(newItemType != NO_ITEM)
			{
				BAMItem		*pItem = NewItem(newItemType);
				if(!pItem)
				{
					// max item count for this type exceeded
					pMono->Out("Max item count exceeded for this type.\n");
					return(false);
				}

				pItem->SetLimits(0, 0, TILE_WIDTH * VPORT_WIDTH - 1, TILE_HEIGHT * VPORT_HEIGHT - 1);
				pItem->SetRect();
				pItem->SetContext(pWorld->gSelf);
				pWorld->vPort.AddAni(pItem->gSelf);
				pItem->SetTilePos(loopX, loopY);
				pItem->fWorldMapUpdate = false;	// item animations dont need to be drawn on World Map
				gItemMap[loopY][loopX] = pItem->gSelf;

				switch(newItemType)
				{
					case KEY:
					case DOWRY_CHEST:
						pWorld->CheckWinLose(tileX, tileY);
						break;
				}
			}
			else
				gItemMap[loopY][loopX] = grip{};

			return(true);
		}
		while(pWorld->map.GetSurroundingCoords(tileX, tileY, loop1, loopX, loopY));

	}
	return(false);
}

itemType
ItemMgr::RemoveItem(uint32 tileX, uint32 tileY)
{
	BAMItem	*pItem;
	itemType	type;

	pItem = CheckItem(tileX, tileY);
	if(!pItem)
		return(NO_ITEM);
	
	// delete from our master list
	lItems.Delete(grip_to_ptr(pItem->gSelf));

	// delete from ViewPort list
	pWorld->vPort.CutAni(pItem->gSelf);

	gItemMap[tileY][tileX] = grip{};
	type = pItem->type;
	DeleteItem(pItem);
	return(type);
}

BAMItem *
ItemMgr::NewItem(itemType newType)
{
	BAMItem	*pItem;
	int		itemCount;

	itemCount = itemCounts[newType];
	if(itemCount >= maxItemCounts[newType])
		return(0);
	itemCounts[newType] = ++itemCount;

	pItem = new BAMItem(newType);
	lItems.Add(grip_to_ptr(pItem->gSelf));
	return(pItem);
}

void
ItemMgr::DeleteItem(BAMItem *pItem)
{
	if(itemCounts[pItem->type] > 0)
		itemCounts[pItem->type]--;
	ADelete(pItem->gSelf);
}

int
ItemMgr::GetItemRange(itemType theItem)
{
	switch(theItem)
	{
		case TELEPORT_TOME:
			return(9999);
		default:
			return(1);
	}
}

// is an item auto-used, click-used, or click-and-set-target-used
itemUsage
ItemMgr::GetItemUsage(itemType theItem)
{
	switch(theItem)
	{
		case NO_ITEM:
			return(ITEM_NONE);
//			return(ITEM_MANUAL);

		case BOAR_BURGER:
		case BERSERKER_BREW:
		case HEALING_SALVE:
		case TELEPORT_TOME:
		case WEIRD_WAND:
		case MASON_MIX:
			return(ITEM_TARGET);

		case BOG_BOOTS:
		case DAEMONS_BANE:
		case DOWRY_CHEST:
		case DRIFT_DISC:
		case FROST_CAPE:
		case KEY:
		case MAP:
		case MIGHT_MANTLE:
		case SACRED_URN:
		case SHRUB_SPRITE:
		case STORM_BRACERS:
		case TRAILFINDER:
		case VERDANT_SHIELD:
		case VIRTUE_VEIL:
		case MANA_ORB:
		case HARPY_NET:
		default:
			return(ITEM_AUTOMATIC);
	}
}

bool
ItemMgr::CheckItemTarget(itemType item, TargetType target, int targetX,
	int targetY)
{
	switch(item)
	{
		case TELEPORT_TOME:
			switch(target)
			{
				case BUILDING:
				case TERRAIN:
					return(true);
			}
			break;

		case WEIRD_WAND:
			switch(target)
			{
				case TERRAIN:
					if(pWorld->map.GetTerrain(targetX, targetY)->moveRate <= 2)
						return(true);
					break;
			}
			break;

		case MASON_MIX:
			switch(target)
			{
				case UNIT:
				case BUILDING:
					return(true);
			}
			break;

		default:
			switch(target)
			{
				case UNIT:
					return(true);
			}
			break;
	}
	return(false);
}

bool
ItemMgr::IsItemReusable(itemType item)
{
	switch(item)
	{
		case MAP:
		case BERSERKER_BREW:
		case HEALING_SALVE:
		case MANA_ORB:
		case MASON_MIX:
		case BOAR_BURGER:
			return(false);
	}
	return(true);
}

//--------------------
// Item class

BAMItem::BAMItem(itemType newType)
{
	uint32	resNum;

	selfType = ITEM;	// for self-identification
	priorityLevel = PRI_ITEM;
	type = newType;
	
	resNum = pWorld->itemMgr.GetPortAnim(type);

	SetRes(RES_ANIM, resNum, 1);
}

BAMItem::~BAMItem()
{
}

bool
BAMItem::Save(uint16 state, nlohmann::json& j)
{
	JsonStream js{j, (state == DURING_SAVE)};
	js.sync(tileX, "tileX");
	js.sync(tileY, "tileY");
	return false;
}
