# VPP Admin Tools — UI Design System

Reference tokens for the dark admin-tools theme (see concept: dark panels, orange section
headers, blue accents). DayZ `.layout` files have no variables, so **copy these exact values**
into layouts and scripts to keep every menu consistent.

Layout colors are `R G B A` floats (0-1). Script colors are `ARGB(a, r, g, b)` ints (0-255).

---

## 1. Color Palette

### Surfaces

| Token              | Hex     | Layout (`color R G B A`)      | Script `ARGB()`            | Usage |
|--------------------|---------|-------------------------------|----------------------------|-------|
| `bg-window`        | #0E1012 | `0.055 0.063 0.071 0.97`      | `ARGB(247, 14, 16, 18)`    | Window/root background |
| `bg-titlebar`      | #08090A | `0.031 0.035 0.039 1`         | `ARGB(255, 8, 9, 10)`      | Window title bar strip |
| `bg-panel`         | #16181B | `0.086 0.094 0.106 1`         | `ARGB(255, 22, 24, 27)`    | Column/panel surface |
| `bg-panel-header`  | #0A0B0C | `0.039 0.043 0.047 1`         | `ARGB(255, 10, 11, 12)`    | Panel header strips (PLAYERS, ACTIONS...) |
| `bg-row`           | #1B1E22 | `0.106 0.118 0.133 1`         | `ARGB(255, 27, 30, 34)`    | List rows, cards |
| `bg-input`         | #1E2126 | `0.118 0.129 0.149 1`         | `ARGB(255, 30, 33, 38)`    | Edit boxes, search fields |
| `bg-button`        | #22262B | `0.133 0.149 0.169 1`         | `ARGB(255, 34, 38, 43)`    | Default button fill |
| `bg-button-hover`  | #2C3138 | `0.173 0.192 0.220 1`         | `ARGB(255, 44, 49, 56)`    | Button hover fill |
| `border`           | #2A2E33 | `0.165 0.180 0.200 1`         | `ARGB(255, 42, 46, 51)`    | Hairline separators (1-2 px panels) |

### Accents

| Token              | Hex     | Layout (`color R G B A`)      | Script `ARGB()`            | Usage |
|--------------------|---------|-------------------------------|----------------------------|-------|
| `accent-orange`    | #E8A33D | `0.910 0.639 0.239 1`         | `ARGB(255, 232, 163, 61)`  | Section header text, active tab markers |
| `accent-blue`      | #3D7DD6 | `0.239 0.490 0.839 1`         | `ARGB(255, 61, 125, 214)`  | Vital underline bars, links, focus |
| `selection-blue`   | #2D5AA0 | `0.176 0.353 0.627 1`         | `ARGB(160, 45, 90, 160)`   | Selected list row highlight |
| `positive-green`   | #4CAF50 | `0.298 0.686 0.314 1`         | `ARGB(255, 76, 175, 80)`   | Online dots, success, self-name, good vitals |
| `warning-yellow`   | #D9B23D | `0.851 0.698 0.239 1`         | `ARGB(255, 217, 178, 61)`  | Mid vitals, warnings |
| `danger-red`       | #C24545 | `0.761 0.271 0.271 1`         | `ARGB(255, 194, 69, 69)`   | Low vitals, destructive accents |
| `danger-button`    | #5E1F1F | `0.369 0.122 0.122 1`         | `ARGB(255, 94, 31, 31)`    | Kick/Ban button fill |

### Text

| Token              | Hex     | Layout (`"text color" R G B A`) | Script `ARGB()`           | Usage |
|--------------------|---------|---------------------------------|---------------------------|-------|
| `text-primary`     | #FFFFFF | `1 1 1 1`                       | `ARGB(255, 255, 255, 255)`| Values, button labels, titles |
| `text-secondary`   | #9AA0A6 | `0.604 0.627 0.651 1`           | `ARGB(255, 154, 160, 166)`| Field labels, subtitles |
| `text-muted`       | #5C6166 | `0.361 0.380 0.400 1`           | `ARGB(255, 92, 97, 102)`  | Hints, disabled, placeholder |

### Vital threshold tints (script side)

Applied to vital icons/values by percentage of max:

| Range        | Token            | `ARGB()`                  |
|--------------|------------------|---------------------------|
| 75 - 100 %   | `positive-green` | `ARGB(255, 76, 175, 80)`  |
| 50 - 74 %    | `warning-yellow` | `ARGB(255, 217, 178, 61)` |
| 25 - 49 %    | `accent-orange`  | `ARGB(255, 232, 163, 61)` |
| 0 - 24 %     | `danger-red`     | `ARGB(255, 194, 69, 69)`  |

---

## 2. Typography

Font family: **InterDisplay** (ships with the mod, `VPPAdminTools/GUI/Fonts/`).
Reference in layouts as `font "vppadmintools/gui/fonts/InterDisplay-<Weight><Size>"`
(explicit size suffix, e.g. `InterDisplay-SemiBold16`) with `"exact text" 1` so text
renders at native pixel size and stays crisp at any resolution. Only generated sizes
exist (12/14/16/22/28/48) — never reference a size without a matching `.fnt`.

| Role                   | Weight    | Size | Notes |
|------------------------|-----------|------|-------|
| Window title           | SemiBold  | 22   | UPPERCASE, `text-primary` |
| Window subtitle        | Regular   | 14   | `text-secondary` |
| Panel header           | Medium    | 16   | UPPERCASE, `text-primary` |
| Section header         | SemiBold  | 14   | UPPERCASE, `accent-orange` |
| Body / field label     | Regular   | 16   | `text-secondary` for labels |
| Field value            | Medium    | 16   | `text-primary` |
| Button label           | Regular   | 16   | `text-primary` |
| Vital value            | SemiBold  | 16   | tinted by threshold |
| Small print / counters | Regular   | 14   | `text-secondary` |
| GUID value             | Medium    | 12   | exception: stays small to avoid clipping |

Weights available: Thin, Light, Regular, Medium, SemiBold, Bold.
Generated sizes: 12, 14, 16, 22, 28, 48.

---

## 3. Iconography

Custom Lucide-based atlas: `set:vpp_icons image:<name>` (see
`GUI/Textures/vpp_icons.imageset`, generated by `tools/build_icon_atlas.py`).

- Icons are rendered white; tint at runtime/layout with widget `color`.
- Standard sizes: 16 px inline (labels, rows), 20 px buttons, 24 px panel headers.
- `ImageWidgetClass` settings: `mode blend`, `"src alpha" 1`, `filter 1`, `"stretch mode" stretch_w_h`.

Icon usage map (Player Manager):

| Purpose                  | Sprite                |
|--------------------------|-----------------------|
| Player Manager / players | `users`, `user`       |
| Search                   | `search`              |
| Refresh                  | `refresh_cw`          |
| Close                    | `x`                   |
| Copy to clipboard        | `copy`                |
| Section chevrons         | `chevron_down`, `chevron_up` |
| Online dot               | `dot` (filled circle) |
| Health                   | `heart_pulse`         |
| Blood                    | `droplets`            |
| Shock                    | `zap`                 |
| Water                    | `glass_water`         |
| Energy                   | `flame`               |
| Heal                     | `heart`               |
| Kill                     | `skull`               |
| Send message             | `message_square`      |
| Spectate                 | `eye`                 |
| Invisible                | `eye_off`             |
| Teleport to player       | `arrow_up_from_line`  |
| Bring player             | `arrow_down_to_line`  |
| Return player            | `rotate_ccw`          |
| Freeze                   | `snowflake`           |
| Force vomit              | `activity`            |
| God mode                 | `shield`              |
| Unlimited ammo           | `infinity`            |
| Set scale                | `scaling`             |
| Kick                     | `log_out`             |
| Ban                      | `ban`                 |
| Weapon in hands          | `sword`               |
| Temperature              | `thermometer`         |
| Apply / confirm          | `check`               |
| Move / drag              | `move`                |
| Pin                      | `pin`                 |
| Gavel (admin action)     | `gavel`               |

---

## 4. Widget Styles (`vpp_widgets.styles`)

Custom widget chrome, replacing vanilla `Editor` / `editor_quad_dark` / `Default` styles.
Defined in `GUI/Styles/vpp_widgets.styles` (registered via `class widgetStyles` in
`assets/config.cpp`), drawing pre-colored sprites from the `vpp_ui` imageset
(generated by `tools/build_ui_atlas.py`). Because the sprites carry the design-token
colors, widgets using these styles take `color 1 1 1 1` — no per-widget tinting.
Tinting still multiplies, so `VPPFill` (pure white) is the style to use when a script
or layout must drive the color.

| Style            | Widget type      | States covered                              | Use for |
|------------------|------------------|---------------------------------------------|---------|
| `VPPButton`      | ButtonWidget     | Normal / Highlight / Pushed / Focus / Disabled | Standard action buttons |
| `VPPButtonDanger`| ButtonWidget     | same                                        | Destructive actions (Kick, Ban) |
| `VPPButtonGhost` | ButtonWidget     | transparent Normal, hover/pushed fill       | Close, refresh, section headers |
| `VPPInput`       | EditBoxWidget    | Normal / Focus (orange border) / Disabled   | Standalone edit boxes |
| `VPPInputBare`   | EditBoxWidget    | all empty                                   | Edit boxes inside a `VPPInputPanel` |
| `VPPInputPanel`  | PanelWidget      | input chrome on a panel                     | Search field wrapper (icon + editbox) |
| `VPPCheckBox`    | CheckBoxWidget   | Normal / Highlight / Mark / Disabled        | All checkboxes |
| `VPPSlider`      | SliderWidget     | Normal / Focus / Disabled, blue fill bar    | Modifier sliders |
| `VPPPanel`       | PanelWidget      | bordered `bg-panel` fill                    | Window + column panels |
| `VPPHeader`      | PanelWidget      | flat `bg-row` fill                          | Cards/strips at #1B1E22 |
| `VPPCard`        | PanelWidget      | flat `bg-input`-dark fill                   | Recessed wells |
| `VPPFill`        | PanelWidget      | flat white fill (tint with `color`)         | Script-colored vital bars (too small to round) |
| `VPPFillRound`   | PanelWidget      | rounded white fill (tint with `color`)      | Header strips, cards, player rows |
| `VPPScroll`      | ScrollWidget     | invisible chrome                            | Scroll areas (like vanilla `blank`) |

9-slice sprite geometry ("glass" theme): rounded 16x16 blocks sliced into 6px corners
(5px radius, antialiased), 1px borders and stretchable edges/center
(`vpp_btn_tl`, `vpp_btn_t`, ... `vpp_btn_br`). Fills are semi-transparent
(alpha 190-235) so panels and buttons read as glass over the 3D scene. `vpp_panel`
and `vpp_input` carry a thin white hairline border (alpha 56/80) for definition;
the slider fill bar uses the rounded `vpp_accent` family. Patch families:
`vpp_btn`, `vpp_btn_hover`, `vpp_btn_push`, `vpp_btn_danger`, `vpp_btn_danger_hover`,
`vpp_input`, `vpp_input_focus`, `vpp_panel`, `vpp_round` (borderless white, tintable),
`vpp_accent` (rounded blue). Flat swatches: `vpp_fill_header`,
`vpp_fill_track`, `vpp_fill_accent`, `vpp_fill_white`, `vpp_fill_transparent`.
Checkboxes: `vpp_check_off`, `vpp_check_off_hover`, `vpp_check_on`, `vpp_check_disabled` (26x26).

To change the look of every button/input/panel at once: edit the color constants in
`tools/build_ui_atlas.py`, re-run it, and re-convert `vpp_ui.png` to `vpp_ui.edds`
(no mipmaps / point filtering so the rounded borders stay crisp).

---

## 5. Layout Metrics

| Token                | Value      | Usage |
|----------------------|------------|-------|
| Title bar height     | 36 px      | exact size (`vexactsize 1`, `size 1 36`) |
| Panel header height  | 28 px      | exact size |
| Section header height| 24 px      | exact size |
| Row height           | 26 px      | player list rows |
| Button height        | 28 px      | action buttons |
| Panel gap            | 8 px       | between columns/panels |
| Panel padding        | 8-10 px    | inner content inset |

## 6. Conventions

- Structure panels as: header strip (`bg-panel-header`) + body (`bg-panel`).
- Prefer `VPP*` widget styles over vanilla ones; only fall back to `blank`/`Colorable`
  for invisible structural widgets.
- Destructive actions (Kick, Ban) use `VPPButtonDanger` with `text-primary`.
- Disabled buttons: engine dims them; avoid manual alpha fighting `Enable()`.
- Prefab layouts for new UI live in `GUI/Layouts/UIHelpers/DesignSystem/`.
- Collapsible sections use `VPPCollapsibleSection` (5_Mission `GUI/UIHelpers`).
- Avoid `color 1 0 0 1` debug markers on root frames; roots should be `0 0 0 0` or a token.
