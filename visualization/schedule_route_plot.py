#!/usr/bin/env python3
import html
import json
import math
import os
import sys


PALETTE = [
    "#2563eb",
    "#dc2626",
    "#16a34a",
    "#9333ea",
    "#ea580c",
    "#0891b2",
    "#be123c",
    "#4f46e5",
    "#65a30d",
    "#a16207",
    "#0f766e",
    "#7c3aed",
]


def compact_label(text: str, max_len: int = 16) -> str:
    replacements = {
        "global_avg_pool": "avg_pool",
        "batch_norm": "bn",
        "bias_add": "bias",
        "residual_add": "res_add",
        "skip_projection": "skip_proj",
        "relu_reduce_sum": "relu_sum",
        "write_reduce_sum": "write_sum",
        "fc_matmul": "fc_mm",
        "input_read": "input",
        "logits_write": "logits",
        "write_activation": "write_act",
        "read_activation": "read_act",
    }
    compact = text
    for old, new in replacements.items():
        compact = compact.replace(old, new)
    if len(compact) <= max_len:
        return compact
    return compact[: max_len - 1] + "."


def estimate_text_width(text: str, size: int) -> float:
    return max(8.0, len(text) * size * 0.58)


def nice_int_step(span: int, target_ticks: int) -> int:
    if span <= target_ticks:
        return 1
    raw = max(1, math.ceil(span / target_ticks))
    for step in (1, 2, 5, 10, 20, 50, 100):
        if raw <= step:
            return step
    return raw


def cycle_at_pos(edge, pos):
    if edge["direction"] == "east":
        return edge["phase"] + pos
    return edge["phase"] - pos


def route_points(edge, src, dst, used_positions):
    lo = min(src["pos"], dst["pos"])
    hi = max(src["pos"], dst["pos"])
    positions = [pos for pos in used_positions if lo <= pos <= hi]
    if src["pos"] not in positions:
        positions.append(src["pos"])
    if dst["pos"] not in positions:
        positions.append(dst["pos"])
    positions = sorted(set(positions), reverse=dst["pos"] < src["pos"])
    return [(cycle_at_pos(edge, pos), pos) for pos in positions]


def svg_text(x, y, text, size=12, anchor="middle", color="#111827", weight="400"):
    return (
        f'<text x="{x:.2f}" y="{y:.2f}" font-size="{size}" '
        f'font-family="Arial, sans-serif" text-anchor="{anchor}" '
        f'font-weight="{weight}" fill="{color}">{html.escape(text)}</text>'
    )


def svg_text_box(
    x,
    y,
    text,
    size=11,
    anchor="middle",
    color="#111827",
    weight="400",
    fill="#ffffff",
):
    width = estimate_text_width(text, size) + 6
    height = size + 5
    if anchor == "middle":
        rect_x = x - width / 2
    elif anchor == "end":
        rect_x = x - width
    else:
        rect_x = x
    rect_y = y - size - 2
    return "\n".join(
        [
            f'<rect x="{rect_x:.2f}" y="{rect_y:.2f}" width="{width:.2f}" '
            f'height="{height:.2f}" rx="3" fill="{fill}" opacity="0.86"/>',
            svg_text(x, y, text, size=size, anchor=anchor, color=color, weight=weight),
        ]
    )


def assign_label_lanes(records, gap=8):
    lanes = []
    for record in sorted(records, key=lambda r: (r["x"], r["width"])):
        placed = False
        for lane_index, lane_end in enumerate(lanes):
            if record["x"] - record["width"] / 2 > lane_end + gap:
                record["lane"] = lane_index
                lanes[lane_index] = record["x"] + record["width"] / 2
                placed = True
                break
        if not placed:
            record["lane"] = len(lanes)
            lanes.append(record["x"] + record["width"] / 2)
    return records


def label_box(x, y, text, size=10, anchor="middle"):
    width = estimate_text_width(text, size) + 8
    height = size + 6
    if anchor == "middle":
        x0 = x - width / 2
    elif anchor == "end":
        x0 = x - width
    else:
        x0 = x
    y0 = y - size - 3
    return (x0, y0, x0 + width, y0 + height)


def boxes_overlap(a, b, pad=3):
    return not (
        a[2] + pad < b[0]
        or b[2] + pad < a[0]
        or a[3] + pad < b[1]
        or b[3] + pad < a[1]
    )


def box_inside(box, left, top, right, bottom):
    return box[0] >= left and box[1] >= top and box[2] <= right and box[3] <= bottom


def place_label(base_x, base_y, text, size, candidates, placed_boxes, bounds):
    left, top, right, bottom = bounds
    fallback = None
    for dx, dy in candidates:
        x = base_x + dx
        y = base_y + dy
        box = label_box(x, y, text, size)
        if not box_inside(box, left, top, right, bottom):
            continue
        if fallback is None:
            fallback = (x, y, box)
        if any(boxes_overlap(box, other) for other in placed_boxes):
            continue
        placed_boxes.append(box)
        return x, y, box
    if fallback is not None:
        x, y, box = fallback
        placed_boxes.append(box)
        return x, y, box
    x = min(max(base_x, left + 20), right - 20)
    y = min(max(base_y, top + 14), bottom - 6)
    box = label_box(x, y, text, size)
    placed_boxes.append(box)
    return x, y, box


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <solution.json> <output.svg>", file=sys.stderr)
        return 1

    with open(sys.argv[1], "r", encoding="utf-8") as f:
        data = json.load(f)

    ops = data["operations"]
    edges = data["edges"]
    if not ops:
        print("solution contains no operations", file=sys.stderr)
        return 1

    op_by_id = {op["id"]: op for op in ops}

    route_point_sets = []
    all_cycles = [0]
    used_positions = sorted({op["pos"] for op in ops})
    for op in ops:
        all_cycles.extend([op["issue"], op["finish"]])
    for edge in edges:
        src = op_by_id[edge["src"]]
        dst = op_by_id[edge["dst"]]
        points = route_points(edge, src, dst, used_positions)
        arrival_cycle = points[-1][0]
        if dst["issue"] > arrival_cycle:
            points.append((dst["issue"], dst["pos"]))
        route_point_sets.append(points)
        all_cycles.extend(c for c, _ in points)

    min_cycle = 0
    max_cycle = max(all_cycles)
    if len(used_positions) == 1:
        used_positions = [used_positions[0] - 1, used_positions[0], used_positions[0] + 1]
    pos_to_row = {pos: row for row, pos in enumerate(used_positions)}

    left = 70
    right = 30
    top = 38
    bottom = 58
    cycle_span = max_cycle - min_cycle + 1
    plot_width = min(1800, max(640, cycle_span * 18))
    plot_height = max(340, (len(used_positions) - 1) * 110)
    width = left + plot_width + right
    height = top + plot_height + bottom

    def sx(cycle):
        return left + (cycle - min_cycle) * plot_width / max(1, max_cycle - min_cycle)

    def sy(pos):
        row = pos_to_row[pos]
        return top + (len(used_positions) - 1 - row) * plot_height / max(1, len(used_positions) - 1)

    x_step = nice_int_step(max_cycle - min_cycle, 14)
    x_ticks = list(range(0, max_cycle + 1, x_step))
    if x_ticks[-1] != max_cycle:
        x_ticks.append(max_cycle)
    y_ticks = used_positions

    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width:.0f}" height="{height:.0f}" viewBox="0 0 {width:.0f} {height:.0f}">',
        '<rect width="100%" height="100%" fill="white"/>',
        svg_text(width / 2, 22, f"Schedule and Data Routes, makespan={data.get('makespan', max_cycle)}", 16, weight="700"),
        f'<rect x="{left}" y="{top}" width="{plot_width}" height="{plot_height}" fill="#f8fafc" stroke="#cbd5e1"/>',
    ]

    for tick in x_ticks:
        x = sx(tick)
        parts.append(f'<line x1="{x:.2f}" y1="{top}" x2="{x:.2f}" y2="{top + plot_height}" stroke="#e2e8f0"/>')
        parts.append(f'<line x1="{x:.2f}" y1="{top + plot_height}" x2="{x:.2f}" y2="{top + plot_height + 5}" stroke="#334155"/>')
        parts.append(svg_text(x, top + plot_height + 20, str(tick), 11))

    for tick in y_ticks:
        y = sy(tick)
        parts.append(f'<line x1="{left}" y1="{y:.2f}" x2="{left + plot_width}" y2="{y:.2f}" stroke="#e2e8f0"/>')
        parts.append(f'<line x1="{left - 5}" y1="{y:.2f}" x2="{left}" y2="{y:.2f}" stroke="#334155"/>')
        parts.append(svg_text(left - 10, y + 4, str(tick), 11, anchor="end"))

    parts.append(svg_text(left + plot_width / 2, height - 14, "cycle", 13, weight="700"))
    parts.append(
        f'<text x="18" y="{top + plot_height / 2:.2f}" font-size="13" '
        'font-family="Arial, sans-serif" font-weight="700" fill="#111827" '
        f'text-anchor="middle" transform="rotate(-90 18 {top + plot_height / 2:.2f})">pos</text>'
    )

    placed_label_boxes = []
    label_bounds = (left + 4, top + 4, left + plot_width - 4, top + plot_height - 4)

    # Draw routes first so operation bars remain visually dominant.
    edge_label_records = []
    for index, edge in enumerate(edges):
        color = PALETTE[index % len(PALETTE)]
        points = route_point_sets[index]
        polyline = " ".join(f"{sx(c):.2f},{sy(p):.2f}" for c, p in points)
        parts.append(
            f'<polyline points="{polyline}" fill="none" stroke="{color}" '
            'stroke-width="2.2" stroke-dasharray="7 5" stroke-linecap="round" '
            f'stroke-linejoin="round" opacity="0.85"><title>'
            f'{html.escape(edge["src"])} -&gt; {html.escape(edge["dst"])}, '
            f'stream {edge["stream"]}</title></polyline>'
        )
        mid_cycle, mid_pos = points[len(points) // 2]
        label = f"e{index + 1}"
        edge_label_records.append(
            {
                "x": sx(mid_cycle),
                "base_y": sy(mid_pos),
                "text": label,
                "width": estimate_text_width(label, 10) + 6,
                "color": color,
            }
        )

    edge_candidates = []
    for dy in (-14, 18, -30, 34, -46, 50):
        for dx in (0, 18, -18, 36, -36, 54, -54):
            edge_candidates.append((dx, dy))

    for record in edge_label_records:
        label_x, label_y, _ = place_label(
            record["x"],
            record["base_y"],
            record["text"],
            10,
            edge_candidates,
            placed_label_boxes,
            label_bounds,
        )
        parts.append(
            f'<line x1="{record["x"]:.2f}" y1="{record["base_y"]:.2f}" '
            f'x2="{label_x:.2f}" y2="{label_y - 8:.2f}" '
            'stroke="#94a3b8" stroke-width="0.8"/>'
        )
        parts.append(
            svg_text_box(
                label_x,
                label_y,
                record["text"],
                size=10,
                color="#111827",
                weight="700",
                fill="#ffffff",
            )
        )

    bar_height = min(46, max(26, plot_height / max(4, len(used_positions) + 1) * 0.9))
    op_label_records = []
    for index, op in enumerate(ops):
        color = PALETTE[index % len(PALETTE)]
        x0 = sx(op["issue"])
        x1 = sx(op["finish"])
        if x1 <= x0:
            x1 = x0 + 5
        y = sy(op["pos"]) - bar_height / 2
        parts.append(
            f'<rect x="{x0:.2f}" y="{y:.2f}" width="{x1 - x0:.2f}" '
            f'height="{bar_height:.2f}" rx="4" fill="{color}" opacity="0.9">'
            f'<title>{html.escape(op["id"])}: issue {op["issue"]}, finish {op["finish"]}, pos {op["pos"]}</title>'
            '</rect>'
        )
        label = compact_label(op["id"])
        op_label_records.append(
            {
                "x": (x0 + x1) / 2,
                "base_y": sy(op["pos"]),
                "row": pos_to_row[op["pos"]],
                "text": label,
                "width": estimate_text_width(label, 10) + 6,
                "bar_width": x1 - x0,
            }
        )

    for record in sorted(op_label_records, key=lambda r: (r["row"], r["x"])):
        if record["bar_width"] >= record["width"] + 10:
            candidates = [(0, 4), (0, -bar_height / 2 - 14), (0, bar_height / 2 + 20)]
        else:
            direction = 1 if record["row"] >= len(used_positions) / 2 else -1
            candidates = []
            for lane in range(7):
                dy = direction * (bar_height / 2 + 18 + lane * 15)
                candidates.extend([(0, dy), (18, dy), (-18, dy), (36, dy), (-36, dy)])
            other = -direction
            for lane in range(5):
                dy = other * (bar_height / 2 + 18 + lane * 15)
                candidates.extend([(0, dy), (18, dy), (-18, dy)])

        label_x, label_y, _ = place_label(
            record["x"],
            record["base_y"],
            record["text"],
            10,
            candidates,
            placed_label_boxes,
            label_bounds,
        )
        if abs(label_y - record["base_y"]) > bar_height / 2 + 5:
            parts.append(
                f'<line x1="{record["x"]:.2f}" y1="{record["base_y"]:.2f}" '
                f'x2="{label_x:.2f}" y2="{label_y - 10:.2f}" '
                'stroke="#94a3b8" stroke-width="0.8"/>'
            )
        parts.append(
            svg_text_box(
                label_x,
                label_y,
                record["text"],
                size=10,
                color="#111827",
                weight="700",
                fill="#ffffff",
            )
        )

    parts.append("</svg>")

    os.makedirs(os.path.dirname(sys.argv[2]) or ".", exist_ok=True)
    with open(sys.argv[2], "w", encoding="utf-8") as f:
        f.write("\n".join(parts))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
