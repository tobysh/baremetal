Implement `Check(image) Report`. `Image` has `NBlocks`, `DataStart`, a bitmap `Used` (one bool per block, metadata included) and `Inodes`, a list of block-number lists (one per inode, possibly empty or `nil`). The data region is `[DataStart, NBlocks)`.

- **Geometry.** If `NBlocks < 0`, `DataStart < 0`, `DataStart > NBlocks` or `len(Used) != NBlocks`, return `Report{BadGeometry: true}` with every list empty, whatever the inodes contain. `DataStart == NBlocks` (no data blocks) is valid.
- **Invalid.** Every pointer outside the data region (negative, in metadata, or `>= NBlocks`) adds one entry to `Invalid`, **including repeats**. `Invalid` is sorted ascending and may contain duplicates.
- **For each valid data block** `b`, with `refs` the number of pointers naming it across all inodes (a block listed twice in one inode counts twice):
  - `Orphan`: `Used[b]` and `refs == 0`;
  - `Dangling`: `!Used[b]` and `refs >= 1`;
  - `Duplicate`: `refs >= 2`, whatever the bitmap says.
- `Orphan`, `Dangling` and `Duplicate` are ascending with no repeats. Bitmap bits below `DataStart` are ignored.
- Never modify the image (including `Used` and the inode slices). An image with no problems yields empty lists. A list with no findings may be `nil` or empty.

The tests compare against a brute-force model over random images and check hand-built cases for each rule.
