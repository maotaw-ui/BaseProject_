# BaseProject_Maotaw

This build keeps the uploaded PixelMenu UI/style and replaces the old menu option tree with the visual options from the current cheat.

Controls:
- F11: show/hide menu
- Up/Down: select row
- Left/Right: change/toggle selected option
- Enter: open/close feature settings
- Drag the small menu header with the mouse to move it
- Delete: exit

Options:
- Master visuals
- Box: enable, style, width %, thickness, fill opacity, box color, fill color
- Healthbar: enable, style, position, width, spacing, segments, foreground/background colors
- Health text
- Skeleton: enable, thickness, color
- Snapline: enable, anchor, thickness, color

Settings auto-save under LocalAppData\BaseProject_Maotaw.

## Added in this update

- Radar is a top-level menu next to Visuals: enable, size, range, marker color, background color
- Trail: enable, width, length, color
- Health Text is now inside Healthbar settings. It uses a tiny 3x5 pixel font, is hidden at 100 HP, and follows the health fill edge downward on left/right bars.

- Healthbar now has numeric Radius 0-6 with real rounded rendering. Radar Distance is directly below Radar, shows meters, and uses 20m steps from 20m to 500m.

- Main menu DISTANCE controls ESP/trail draw range from 20M to 500M in 20M steps.
- Healthbar thickness/spacing scale with projected player size; radius uses thin-bar elliptical caps.

## Armor / bar outline update
- Armor is now a full ESP bar with Normal / Segmented / Gradient styles.
- Armor position: Left / Right / Top / Bottom.
- Armor has text, radius, width, spacing, segments, foreground/background colors.
- Healthbar and Armor both have configurable outline thickness (0-5) and outline color.
- Armor value is refreshed from `m_ArmorValue` every frame.

## Box outline update
- Box settings now include OUTLINE thickness (0-5) and OUTLINE COLOR.
- Outline applies to Box, Cornered, Filled, Rainbow, and Rainbow Filled styles.
- Thickness 0 disables the outline.
