import re
import subprocess
import tempfile
import unittest
import json
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
BFF_BIN = PROJECT_ROOT / "bff"

def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")

class TEST_BFF(unittest.TestCase):

    def bff(self, *args: str, cwd: Path):
        return subprocess.run([str(BFF_BIN), *args], cwd=str(cwd), text=True, capture_output=True)

    def test_init(self):
        with tempfile.TemporaryDirectory() as td:
            repo = Path(td)

            self.bff("init", cwd=repo)
            self.assertTrue((repo / ".bff").is_dir())

            self.bff("init", cwd=repo)
            self.assertTrue((repo / ".bff").is_dir())

    def test_compare(self):
        with tempfile.TemporaryDirectory() as td:
            base = Path(td)
            repo_a = base / "a"
            repo_b = base / "b"
            repo_a.mkdir()
            repo_b.mkdir()

            # Same relative path + same content
            write_text(repo_a / "a.txt", "same")
            write_text(repo_b / "a.txt", "same")

            # Unique by path and content
            write_text(repo_a / "onlyA.txt", "A")
            write_text(repo_b / "onlyB.txt", "B")

            self.bff("init", cwd=repo_a)
            self.bff("index", cwd=repo_a)
            self.bff("init", cwd=repo_b)
            self.bff("index", cwd=repo_b)

            proc = self.bff("compare", str(repo_b), cwd=repo_a)
            out = proc.stdout

            def num(label: str) -> int:
                m = re.search(rf"{re.escape(label)}:\s*(\d+)", out)
                self.assertIsNotNone(m, msg=f"Missing '{label}' in output:\n{out}")
                return int(m.group(1))

            self.assertEqual(num("Files in this not in other"), 1)
            self.assertEqual(num("Files in other not in this"), 1)
            self.assertEqual(num("Same path + same content"), 1)
            self.assertEqual(num("Unique contents in this not in other"), 1)
            self.assertEqual(num("Unique contents in other not in this"), 1)
            self.assertEqual(num("Common contents (unique hashes)"), 1)

    def test_index(self):
        with tempfile.TemporaryDirectory() as td:
            repo = Path(td)

            write_text(repo / "hello.txt", "HELLO")
            write_text(repo / "sub/world.txt", "WORLD")

            self.assertEqual(self.bff("init", cwd=repo).returncode, 0)
            proc = self.bff("index", cwd=repo)
            self.assertEqual(proc.returncode, 0, msg=proc.stderr)
            self.assertIn("Indexed files into .bff/index.json", proc.stdout)

            index_path = repo / ".bff/index.json"
            self.assertTrue(index_path.exists(), msg=f"Expected {index_path} to exist")

            payload = json.loads(index_path.read_text(encoding="utf-8"))
            self.assertIn("files", payload)
            all_paths = set()
            for paths in payload["files"].values():
                all_paths.update(paths)
            self.assertIn("hello.txt", all_paths)
            self.assertIn(str(Path("sub") / "world.txt"), all_paths)

    def test_status(self):
        with tempfile.TemporaryDirectory() as td:
            repo = Path(td)

            write_text(repo / "a.txt", "A")

            self.assertEqual(self.bff("init", cwd=repo).returncode, 0)
            self.assertEqual(self.bff("index", cwd=repo).returncode, 0)

            proc = self.bff("status", cwd=repo)
            self.assertEqual(proc.returncode, 0, msg=proc.stderr)
            self.assertIn("New files:", proc.stdout)
            self.assertIn(str(Path(".bff") / "index.json"), proc.stdout)

            write_text(repo / "new.txt", "NEW")
            proc = self.bff("status", cwd=repo)
            self.assertEqual(proc.returncode, 0, msg=proc.stderr)
            self.assertIn("New files:", proc.stdout)
            self.assertIn("new.txt", proc.stdout)

            (repo / "new.txt").unlink()
            proc = self.bff("status", cwd=repo)
            self.assertEqual(proc.returncode, 0, msg=proc.stderr)
            self.assertIn("New files:", proc.stdout)
            self.assertIn(str(Path(".bff") / "index.json"), proc.stdout)

            (repo / "a.txt").unlink()
            proc = self.bff("status", cwd=repo)
            self.assertEqual(proc.returncode, 0, msg=proc.stderr)
            self.assertIn("Deleted files:", proc.stdout)
            self.assertIn("a.txt", proc.stdout)

            write_text(repo / "a.txt", "A2")
            proc = self.bff("status", cwd=repo)
            self.assertEqual(proc.returncode, 0, msg=proc.stderr)
            self.assertIn("Modified files:", proc.stdout)
            self.assertIn("a.txt", proc.stdout)

    def test_match(self):
        with tempfile.TemporaryDirectory() as td:
            base = Path(td)
            dest = base / "dest"
            src = base / "src"
            dest.mkdir()
            src.mkdir()

            write_text(src / "dirA/common.txt", "COMMON")
            write_text(dest / "dirB/common_copy.txt", "COMMON")

            # match should copy the copy file in the same directory as newfile.txt
            write_text(src / "dirA/newfile.txt", "NEW")

            self.bff("init", cwd=dest)
            self.bff("init", cwd=src)
            self.bff("index", cwd=dest)
            self.bff("index", cwd=src)

            proc = self.bff("match", str(src), cwd=dest)
            self.assertIn("Copied 1 files", proc.stdout)

            copied_path = dest / "dirB/newfile.txt"
            self.assertTrue(copied_path.exists(), msg=f"Expected {copied_path} to exist")
            self.assertEqual(copied_path.read_text(encoding="utf-8"), "NEW")

if __name__ == "__main__":
    unittest.main(verbosity=2)
