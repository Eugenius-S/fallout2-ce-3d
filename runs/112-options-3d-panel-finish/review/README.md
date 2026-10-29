# Task 112 visual evidence

The screen sheets pair the available before captures with the final after captures at 16:9 and 4:3. The detail sheets show the panel edge and scrollbar track.

After captures use the task worktree build. The 16:9 top before capture is from commit `9c82d409`, before this task's source edits. The 16:9 bottom and both 4:3 before captures reuse the Task 017 matrix from `e29ef058`, because no current-main bottom/4:3 captures existed before editing. That commit is not an ancestor of current `origin/main`, so those three before images are visual context rather than exact current-main baselines.

Relevant metadata:

- Current 16:9 top before: 1920x1080, logical 960x540, scale 2, EXE SHA-256 `9ad1c156cd18843d516b7d2d871f911bd80d5ef0c8027d7b58c5872a6e1c6957`.
- After captures: 16:9 is 1920x1080 at scale 2; 4:3 is 1920x1440 at logical 640x480, scale 3. Their JSON sidecars sit beside each raw PNG.
- Historical Task 017 before captures: EXE SHA-256 `4c3933c595323a981b0008add8d94ef47b397c77ef709011ab16a49e349aae28`.

Raw PNG captures are not committed. The four JPEG review sheets are each below 1 MB.
