The generator is exactly the one in the lesson. It starts from a secret `uint32_t` state; each call does `state = state * 1664525u + 1013904223u` and returns `state >> 16`.

Write `uint16_t predict_next(uint16_t o1, uint16_t o2, uint16_t o3)`. Given three consecutive outputs, return the fourth. The tests use generators with several different secret seeds.
