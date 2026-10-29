Implement `void release(void **owner)`. For a non-NULL owner address, free `*owner` and replace it with NULL. A NULL owner address or already-NULL value is harmless. Repeated calls through the same owner must be safe.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
