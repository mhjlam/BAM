// UNITLIB.HPP
// Unit librarian class

#ifndef UNITLIB_HPP
#define UNITLIB_HPP

#include "tigre.hpp"

#include "units.hpp"

class UnitGroup : public Object	// class of units (all acolytes, or all rangers, etc)
{
	public:		  
						UnitGroup(void);
						~UnitGroup();
		void			Init(unitType newType, char *parms);
		bool			FreeMovementIn(terrainType terrain), NoMovementIn(terrainType terrain);
		void			GetTerrainBonuses(terrainType thisTerrain, int32 &attackBonus, int32 &rangeBonus,
							int32 &defenseBonus);
		virtual bool Save(uint16 state, FILE *pFile = NULL);
		int32			GetMoveRate(uint32 terrainType);		// ask group's speed if on terrain X

		char			uGroupDataStart;
		unitType		type;
		moveType		movement;		// type of locomotion (swimming, walking, etc)
		int32			maxHP, baseMoveRate, baseAttack, baseDefense, baseAttackRange;
		int32			baseAnim, portAnim, skillRange, skillCost, transformCost, transformTime, expRequired, expGained,
							researchTime;
		char			unitName[20], unitNameInternal[20];

		// pre-computed values for each kind of terrain
		int32			attack[TERRAINMAXTYPES], attackRange[TERRAINMAXTYPES], defense[TERRAINMAXTYPES],
							moveRates[TERRAINMAXTYPES];
		char			uGroupDataEnd;
};

class UnitLib : public TMovable
{
	public:
				UnitLib(void);
				~UnitLib();
		void	Cleanup(void);
		bool	Load(void);
		void	Report(void);
		int	CountUnits(int xCenter, int yCenter, int side, int radius);
		Unit	*GetUnit(int32 x, int32 y);
		void	PlaceUnitGrip(grip gUnit, int32 x, int32 y);
		grip	GetUnitGrip(int32 x, int32 y);
		UnitGroup *GetUnitGroup(unitType type);
		void	AddUnit(grip gUnit, bool fPrevSerialNum = FALSE);			// register a unit
		void	CutUnit(grip gUnit, bool fLeaveSerialNum = FALSE);			// unregister a unit
		grip	NewUnit(void);					// create && register a new unit
		grip	NewUnit(unitType newType, uint32 newSide, uint32 newX, uint32 newY, itemType newItem, action newAction);
		void	DeleteUnit(grip gUnit);		// delete and unregister a unit
		uint32	GetUnitNum(coord X, coord Y);
		unitType	GetUnitType(char *unitName);
		void		SetEnemyFlag(int32 sideA, int32 sideB, bool fNewState);
		bool		CheckEnemyFlag(int32 sideA, int32 sideB);
		virtual bool Save(uint16 state, FILE *pFile = NULL);

		List	lUnits[5];

		char		uLibDataStart;
		uint32	totalUnits;
		grip	gUnitGroups[UNITMAXTYPES];
		grip	gUnitMap[WORLD_HEIGHT][WORLD_WIDTH];
		bool	fEnemyFlags[5][5];
		char		uLibDataEnd;
};

#endif
