Animation priority allocation for BAM
as of June 7, 95
-----------------------------------------

{
	1000
	{
		1000 - north edge

		floor range, things underneath EVERYTHING in tile
		(Juggernaut blood pool)

		1090 - south edge
	}

	1100
	{
		1100 - north edge

		swimmer range, things underneath walkers
		(Turtles)
	
		1200	swimmer center of tile
		1200 - some fixed animations (fountain, waterfall)

		1290 - south edge
	}

	1300
	{
		1300 - north edge

		walker range
		(Acolytes, etc)

		terrain ani range
		(brush, swamp, etc - tied to unit at UNITPRI+2
		(juggernaut blood splat at unit y)

		1400 - walker center of tile
		1400 - ground flames
		1400 - some fixed animations (windmill)
		1401 - items, fixed

		1490 - south edge
	}

	1500
	{
		1500 - north edge

		projectile range
		(Lightning, arrows, etc)

		1600 - projectile center of tile

		1690 - south edge
	}

	1700
	{
		1700 - north edge

		flier range
		(Wyrms, Gargoyles, etc)

		1800 - flier center of tile

		1890 - south edge
	}

	1900 - fireball bursts

	1999
}
