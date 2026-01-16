
## BFF

A tiny, content-based index tool for managing “save” folders (or any directory tree) without relying on file names and content, only hash.

`bff` computes a SHA-1 hash of each file’s contents and stores only those hashes in `.bff/index.json`

Repositories can be nested. The commands are applied recursively to each nested repos.

Json format is used for readability and debug. One single file per repository has all the files informations. But the json file does not include the files of nested bff repos.

## Installation
Requires the JSON library `nlohmann/json`.

```bash
make
sudo make install
```

## Usage

After installing:

```bash
bff init
bff index
```

## Bff Commands

- `init` — Create `.bff/` in the current directory.
- `index` — Scan the directory tree, compute content hashes, and write `.bff/index.json`.
- `clean` — Remove `.bff/index.json`.
- `status` — Compare current files vs the saved index and report `Modified`, `Deleted`, and `New` files.
- `clones` — Print groups of files that share the same content hash (duplicate contents).
- `compare <path>` — Compare this repo’s index to another repo’s index, reporting differences by relative path and by content overlap.
- `match <path>` — Copy files from `<path>` into the current repo when their content hash is missing here. Attempts to map source folders to “best matching” destination folders using hash-based matching.

