Write a `contains_net(a_net,a_mask,b_net,b_mask)` that checks the mask of A is no longer than B's and that B's network masked by A's mask equals A's network.
===
Treat `any` as `0.0.0.0/0` and ports `any` as `0-65535`; protocol `any` covers everything but a specific protocol covers only itself.
