#!/usr/bin/env python3
"""Vision-QA loop: send game screenshots to a smartapi vision model for critique.

Usage:
  python vision_qa.py                                  # latest 4 worldshot_* pngs
  python vision_qa.py --all                            # all pngs in Saved/GrokShots
  python vision_qa.py path/to/shot.png path2.png       # explicit files
  python vision_qa.py --model glm-5.3                  # override vision model

Writes report to Saved/VisionQA/report_<ts>.md and prints it.
Model fallback chain: gpt-6.2-astra -> claude-fable-5-2 -> glm-5.3 (on HTTP/vision errors).
"""
import argparse, base64, io, json, pathlib, sys, time, urllib.request

try:
    from PIL import Image
except ImportError:
    Image = None

HERE = pathlib.Path(__file__).parent
SHOTS = HERE.parent / "Saved" / "GrokShots"
OUTDIR = HERE.parent / "Saved" / "VisionQA"
KEY = (HERE / "_secrets" / "smartapi.key").read_text().strip()
ENDPOINT = "https://api.smartapi.pro/v1/chat/completions"
FALLBACKS = ["gpt-6.2-astra", "claude-fable-5-2", "glm-5.3"]

PROMPT = """You are a senior visual QA reviewer for a dark-fantasy AAA ARPG (Unreal Engine 5, target bar: Diablo IV / Black Myth Wukong presentation).
Critique this in-game screenshot. Judge ONLY what is visible.

Answer in markdown, exactly this structure:

## <screenshot name>
**Verdict:** <one line: does this frame look like a AAA dark-fantasy game?>

**Problems (worst first):**
1. **[P0|P1|P2]** <problem> — <what to change>
2. ...
3. ...

**Quick win:** <single cheapest change with the biggest visible payoff>

Scale: P0 = ruins believability (unreadable gameplay space, broken visuals), P1 = clearly below target bar, P2 = polish.
Be concrete (lighting direction, fog density, material response, composition, color palette). No generic advice.
"""


def shrink(path: pathlib.Path, max_w: int, quality: int) -> bytes:
    """Downscale to JPEG bytes so the base64 payload stays small (empty-content guard)."""
    if Image is None:
        return path.read_bytes()
    img = Image.open(path).convert("RGB")
    if img.width > max_w:
        img = img.resize((max_w, round(img.height * max_w / img.width)), Image.LANCZOS)
    buf = io.BytesIO()
    img.save(buf, "JPEG", quality=quality)
    return buf.getvalue()


def ask(model: str, b64: str, name: str, max_tokens: int) -> str:
    body = json.dumps({
        "model": model,
        "messages": [{"role": "user", "content": [
            {"type": "text", "text": PROMPT + f"\nScreenshot file name: {name}\n"},
            {"type": "image_url", "image_url": {"url": f"data:image/jpeg;base64,{b64}"}},
        ]}],
        "max_tokens": max_tokens,
        "temperature": 0.2,
    }).encode("utf-8")
    req = urllib.request.Request(ENDPOINT, data=body, headers={
        "Authorization": "Bearer " + KEY,
        "Content-Type": "application/json",
    })
    with urllib.request.urlopen(req, timeout=600) as r:
        resp = json.load(r)
    ch = resp["choices"][0]
    text = (ch["message"].get("content") or ch["message"].get("reasoning_content") or "").strip()
    usage = resp.get("usage", {})
    print(f"  [{model}] {name}: in={usage.get('prompt_tokens')} out={usage.get('completion_tokens')} finish={ch.get('finish_reason')}", flush=True)
    if not text:
        raise RuntimeError(f"empty content (finish={ch.get('finish_reason')})")
    return text


def review(path: pathlib.Path, model: str, max_tokens: int, max_w: int) -> str:
    raw = shrink(path, max_w, 85)
    b64 = base64.b64encode(raw).decode()
    print(f"  payload: {len(raw)//1024} KB -> {len(b64)//1024} KB b64", flush=True)
    chain = list(dict.fromkeys([model] + FALLBACKS))
    last_err = None
    for m in chain:
        for attempt in range(2):  # empty-content retry once per model
            try:
                return ask(m, b64, path.name, max_tokens)
            except RuntimeError as e:
                last_err = e
                print(f"  retry: {e}", flush=True)
                time.sleep(4)
            except Exception as e:  # HTTP/vision unsupported -> next model
                last_err = e
                print(f"  model {m} failed: {e}", flush=True)
                break
    return f"(review failed for {path.name}: {last_err})"


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="*", help="screenshot files; empty = auto-pick")
    ap.add_argument("--all", action="store_true", help="all pngs in Saved/GrokShots")
    ap.add_argument("--latest", type=int, default=4, help="auto-pick: newest N worldshot_* (default 4)")
    ap.add_argument("--model", default="gpt-6.2-astra")
    ap.add_argument("--max-tokens", type=int, default=3000)
    ap.add_argument("--max-width", type=int, default=1600)
    ap.add_argument("--out", default="")
    a = ap.parse_args()

    if a.paths:
        shots = [pathlib.Path(p) for p in a.paths]
    elif a.all:
        shots = sorted(SHOTS.glob("*.png"))
    else:
        shots = sorted(SHOTS.glob("worldshot_*.png"), key=lambda p: p.stat().st_mtime)[-a.latest:]
    if not shots:
        sys.exit("no screenshots found (run -GrokWorldShot first, or pass paths)")
    print(f"reviewing {len(shots)} shot(s): {[p.name for p in shots]}", flush=True)

    parts = [f"# Vision-QA report — {time.strftime('%Y-%m-%d %H:%M')}",
             f"model: {a.model} (fallbacks: {', '.join(FALLBACKS)})", ""]
    for p in shots:
        print(f"-> {p.name}", flush=True)
        parts.append(review(p, a.model, a.max_tokens, a.max_width))
        parts.append("")

    OUTDIR.mkdir(parents=True, exist_ok=True)
    out = pathlib.Path(a.out) if a.out else OUTDIR / f"report_{time.strftime('%Y%m%d_%H%M')}.md"
    report = "\n".join(parts)
    out.write_text(report, encoding="utf-8")
    print(f"\nreport: {out}", flush=True)
    print(report)


if __name__ == "__main__":
    main()
