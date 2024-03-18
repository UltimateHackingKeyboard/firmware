Removes the following warnings from the build log:

---------------------------------------------------------
--- WARNING: Using default MCUBoot key, it should not ---
--- be used for production.                           ---
---------------------------------------------------------

---------------------------------------------------------------------
--- WARNING: Using a bootloader without pm_static.yml.            ---
--- There are cases where a deployed product can consist of       ---
--- multiple images, and only a subset of these images can be     ---
--- upgraded through a firmware update mechanism. In such cases,  ---
--- the upgradable images must have partitions that are static    ---
--- and are matching the partition map used by the bootloader     ---
--- programmed onto the device.                                   ---
---------------------------------------------------------------------