//	ITEMS.HPP
// items for BAM

#ifndef ITEMS_HPP
#define ITEMS_HPP

#include "list.hpp"

#include "alldefs.hpp"
#include "viewport.hpp"

#define SQUIB_ITEM_NAMES	1130

class BAMItem : public BAM_Ani
{
	public:
			BAMItem(itemType newType);
			~BAMItem();	
		virtual bool Save(uint16 state, FILE *pFile = NULL);

		char		itemDataStart;
		itemType	type;
		char		itemDataEnd;
};

class ItemMgr : public Periodic
{
	public:
				ItemMgr(void);
				~ItemMgr();
	void		Init(void), Cleanup(void);
	uint32	GetPortAnim(itemType item);				// get Anim # for given itemType
	BAMItem *	NewItem(itemType newType);
	void			DeleteItem(BAMItem *pItem);
	BAMItem *	CheckItem(uint32 tileX, uint32 tileY);
	bool		PlaceItem(uint32 tileX, uint32 tileY, itemType newItemType, int dropRadius = 1, bool fSkipCenter = FALSE);
	itemType	RemoveItem(uint32 tileX, uint32 tileY);
	itemType PickRandomItem(void);
	itemUsage GetItemUsage(itemType theItem);
	int		GetItemRange(itemType theItem);
	itemType GetItemType(char *szItem);
	bool		CheckItemTarget(itemType item, TargetType target,
					int targetX = -1, int targetY = -1), IsItemReusable(itemType item);
	virtual bool Save(uint16 state, FILE *pFile = NULL);

	List		lItems;									// list of X,Ys that have items, and their types

	char		iMgrDataStart;
	grip		gItemMap[WORLD_HEIGHT][WORLD_WIDTH];	// map of items sitting on the ground
																// (not carried by a unit, or in a building)
	uint32	attackMods[ENDOFITEMTYPES], attackRangeMods[ENDOFITEMTYPES], defenseMods[ENDOFITEMTYPES],
				moveRateMods[ENDOFITEMTYPES], aniNums[ENDOFITEMTYPES];
	char		itemNames[ENDOFITEMTYPES][30];
	bool		fItemCanAppear[ENDOFITEMTYPES];
	int		itemCounts[ENDOFITEMTYPES], maxItemCounts[ENDOFITEMTYPES];
	char		iMgrDataEnd;
};
#endif
