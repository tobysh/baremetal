Put the mutex and the map in the struct: `mu sync.Mutex` and `counts map[string]int`. `NewSafeCounter` must create the map with `make`.
===
In `Inc` and `Value` call `c.mu.Lock()` followed by `defer c.mu.Unlock()`. Use pointer receivers so the mutex isn't copied.
===
For `ParallelSum`, compute `size := (len(nums) + workers - 1) / workers` and take chunks `nums[lo:hi]` (clamp `hi` to `len(nums)`). Each goroutine writes its partial sum into its own slot `partial[i]`, so no lock is needed; after `wg.Wait()` add the slots up.
