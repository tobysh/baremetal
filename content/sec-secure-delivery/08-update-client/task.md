Implement `Verify`, `Recover`, `Rollback`, `Boot` and `Install` in the starter. Read the doc comment on each; everything else (disk, layout, `atomicWrite`, `raiseHW`, `LoadState`, `Decide`, `ParseMeta`) is supplied and may be used freely. Do not change the supplied types or file formats.

`main` installs 1.0, then crashes halfway through writing 1.1's spare slot, restarts, shows that 1.0 still boots, retries and boots 1.1. Run it to see it.

The hidden tests inject a fault of every kind at every operation of an install, then random multi-fault schedules, then check the five properties from the lesson. They also test tampered, expired, replayed and downgraded packages, health-check rollback (including a package that is unhealthy on a device with no previous version), bit rot in the running slot, and unrecoverable state. Mutating-operation counts are used only to prove that rejected packages cause none.
