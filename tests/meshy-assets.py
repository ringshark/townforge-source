import importlib.util
import io
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("meshy", Path(__file__).resolve().parents[1] / "tools/characters3d/meshy.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


class Assets(unittest.TestCase):
    def test_check_never_posts_or_exposes_key(self):
        requests = []
        def request(req, **kwargs):
            requests.append(req)
            return io.BytesIO(b"[]")
        with patch.dict(os.environ, {"MESHY_OPERATION": "check", "MESHY_API_KEY": "private-test-key"}), patch.object(m.urllib.request, "urlopen", request), patch("sys.stdout", new_callable=io.StringIO) as log:
            m.main()
            self.assertNotIn("private-test-key", log.getvalue())
        self.assertEqual(len(requests), 1)
        self.assertEqual(requests[0].get_method(), "GET")

    def test_download_validates_glb_and_does_not_send_key(self):
        glb = struct.pack("<4sII", b"glTF", 2, 20) + b"12345678"
        task = {"id": "task-1", "status": "SUCCEEDED", "result": {
            "rigged_character_glb_url": "https://assets.meshy.ai/rig.glb?private=signed",
            "basic_animations": {"walking_glb_url": "https://assets.meshy.ai/walk.glb"}}}
        requests = []
        def download(url, **kwargs):
            self.assertIsInstance(url, str)  # no authenticated API Request object
            requests.append(url)
            return io.BytesIO(glb)
        with tempfile.TemporaryDirectory() as temp, patch.object(m.urllib.request, "urlopen", download):
            dest = Path(temp)
            m.stage(task, dest)
            manifest = (dest / "manifest.json").read_text()
            self.assertNotIn("signed", manifest)
            self.assertEqual(len(json.loads(manifest)["files"]), 2)
            self.assertEqual(len(requests), 2)

    def test_reject_invalid_outputs_and_ids(self):
        for url in ["http://assets.meshy.ai/a.glb", "https://user:password@assets.meshy.ai/a.glb"]:
            with self.assertRaises(ValueError):
                m.outputs({"model_urls": {"glb": url}})
        for ident in ["../task", "https://meshy.ai/task", ""]:
            with self.assertRaises(ValueError):
                m.task_id(ident)
        with self.assertRaises(ValueError):
            m.validate_glb(b"<html>error</html>")

    def test_invalid_prompt_does_not_spend(self):
        with patch.dict(os.environ, {"MESHY_OPERATION": "preview", "MESHY_PROMPT": ""}), patch.object(m, "api") as api:
            with self.assertRaises(ValueError):
                m.main()
            api.assert_not_called()

    def test_generation_post_is_not_retried(self):
        with patch.dict(os.environ, {"MESHY_OPERATION": "preview", "MESHY_PROMPT": "One sword"}), patch.object(m, "api", side_effect=ValueError("ambiguous connection")) as api:
            with self.assertRaises(ValueError):
                m.main()
            self.assertEqual(api.call_count, 1)
            self.assertEqual(api.call_args.args[1]["mode"], "preview")


if __name__ == "__main__":
    unittest.main()
