# Task 184 scratch-copy run

Checked 2026-11-05 with RPU v2.4.34, the latest official release at the
time. The official Windows installer URL is
<https://github.com/BGforgeNet/Fallout2_Restoration_Project/releases/download/v2.4.34/rpu_v2.4.34.exe>.
Its release metadata reports SHA-256
`226b2252840a9230bb4ced1550becb2bd69d3580c15ee5c2148d5dc0a750f898`.

The source game folder already contained RPU 2.4.34. Its installed copy was
copied to a separate scratch game directory outside the repository;
`mod-cache`, logs, shots, user assets and saves were excluded. This records a
boot of the installed release in an isolated copy. The installer itself was
not rerun. No game files were added to this repository.

The background `dev-snapshot.ps1 -GameRoot ...` run started a new game on
`ARTEMPLE.MAP`, then loaded `ARVILLAG.MAP` and `KLADWTWN.MAP`. All three
snapshots report the `scene-gl` renderer, an available scene preview, and
nonzero rendered objects and walls. The run exited successfully. The default
test config used Russian at 1920x1080 (logical 960x540). The source and scratch
`ddraw.ini` hashes match; both report `RP 2.4.34`. Scratch `mods/rpu.dat`
SHA-256: `27db830e0fb67d02b60e166e2acb772a92292f5f4dd88f733b740b406a0864ee`.

| Map | Elevation | Objects | Wall triangles | JPEG |
| --- | ---: | ---: | ---: | --- |
| Temple of Trials (`ARTEMPLE`) | 0 | 1082 | 135 | `devhero-20261105-002108-000.jpg` |
| Arroyo Village (`ARVILLAG`) | 0 | 4010 | 2825 | `devhero-20261105-002110-000.jpg` |
| Klamath Town (`KLADWTWN`) | 0 | 4601 | 48385 | `devhero-20261105-002112-000.jpg` |

The matching JSON files preserve snapshot metadata. Original PNGs are kept
outside the repository in the scratch copy's `evidence-raw` directory;
the committed JPEGs are full-resolution conversions, all under 1 MB.

Build executable SHA-256: `d0d9c3ab763311ff64be8e80f5358e44449db8c03577631195158a0f0f00e567`.
CE commit reported by the run: `4146fe56`. CTest passed 20/20; scene-export
unit tests 125/125; regression unit tests 12/12.
