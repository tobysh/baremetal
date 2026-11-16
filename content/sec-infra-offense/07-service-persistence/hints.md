Enabled and Running are different properties. Confirm both after the first modeled reboot.
===
Stop before deletion, audit absence, then reboot again to verify the persistent effect is gone.
===
Create, reboot, then read. Before the reboot `Running` is false, so the checks on the unit state and on `/service-proof` belong after `POST /reboot`.
===
Cleanup is stop, delete, confirm 404, reboot again, then require a 200 with an empty body. Run it whenever the create returned 201, even when the confirmation failed.
