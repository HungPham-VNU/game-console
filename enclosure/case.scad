// Brick-Game-style handheld case for the ES3N28P (2.8" ESP32-S3 display board)
//
// Units: mm. The SHELL is modelled front-face-down: z = 0 is the outside of the
// front face and z grows towards the back. Print the shell like this (no supports).
// The LID is modelled outside-face-down too (z = 0 = outside of the back).
//
// DRAFT v1 – board numbers come from docs/es3n28p/ES3N28P_Size.pdf.
// Measure your real board, switches and battery and adjust the parameters
// before the full print (print the "test_front" part first).

part = "both";   // "shell" | "lid" | "both" | "test_front"

$fn = 48;

/* ---------- Case ---------- */
W = 72;          // outer width
H = 150;         // outer height
D = 24;          // outer depth incl. lid
R = 6;           // outer corner radius
wall  = 2;       // side walls
front = 2;       // front plate
lid_t = 2;       // lid plate

/* ---------- ES3N28P board (mounted rotated 180°, USB-C to the top) ---------- */
bw = 50;  bh = 86;  pcb_t = 1.6;
lcd_stack = 2.8;                     // LCD 2.3 + glue 0.5 -> PCB front face depth
top_gap = 1;                         // board edge to top inner wall
bx0 = (W - bw) / 2;                  // board bottom-left corner (front view)
by0 = H - wall - top_gap - bh;
board_holes = [[4, 4], [46, 4], [4, 82], [46, 82]];
boss_d = 5.5;
// LCD active area in rotated-board coordinates: [x, y, w, h]
aa = [3.3, 17.28, 43.2, 57.6];
win_margin  = 1.0;                   // window larger than the active area
win_chamfer = 1.5;

/* ---------- USB-C on the top wall ---------- */
usb_x = bw / 2;                      // from board left edge – VERIFY on the real board
usb_w = 13;  usb_h = 7;
usb_z = front + lcd_stack + pcb_t + 1.6;

/* ---------- Buttons ---------- */
big_d   = 12.6;                      // hole for 12x12 switch round caps
small_d = 4.2;                       // hole for 6x6 tall-stem switches
dpad = [21, 34];  dpad_off = 10.5;
a_btn = [57, 40];  b_btn = [45, 30];
select_btn = [30, 14];  start_btn = [42, 14];
// Button perfboard (66 x 46 mm) sits on 4 bosses; this is the depth of its FRONT face.
// btn_pcb_z = h - p, where h = height from perfboard to cap top (switch + cap, measured)
// and p = how far the caps should stick out of the front (1.5 mm feels good).
btn_pcb_z = 8.5;
btn_pcb_holes = [[6, 13], [66, 13], [6, 53], [66, 53]];

/* ---------- Side cut-outs ---------- */
sw_y = 112;  sw_z = 15;  sw_size = [9, 4.5];          // slide switch, LEFT wall
sd_enable = true;                                      // SD slot, RIGHT wall – VERIFY
sd_y = by0 + 50;  sd_z = front + lcd_stack + pcb_t + 1.5;  sd_size = [14, 3];

/* ---------- Lid ---------- */
lid_posts = [[wall + 4, wall + 4], [W - wall - 4, wall + 4],
             [wall + 4, H - wall - 4], [W - wall - 4, H - wall - 4]];
post_d = 7.5;
insert_d = 4.0;  insert_len = 6;     // M3 heat-set insert hole
spk_y = 22;  spk_r = 9;              // speaker grille centre / radius
bat = [36, 52];  bat_rib_h = 4;      // battery pocket (603450 + clearance)
bat_y = by0 + bh / 2;

/* ================================================================== */

module rbox(w, h, d, r) {
    linear_extrude(d) offset(r) offset(-r) square([w, h]);
}

module window() {
    x = bx0 + aa[0] - win_margin;
    y = by0 + aa[1] - win_margin;
    w = aa[2] + 2 * win_margin;
    h = aa[3] + 2 * win_margin;
    hull() {
        translate([x - win_chamfer, y - win_chamfer, -0.1])
            cube([w + 2 * win_chamfer, h + 2 * win_chamfer, 0.1]);
        translate([x, y, win_chamfer]) cube([w, h, front]);
    }
}

module button_holes() {
    for (o = [[0, dpad_off], [0, -dpad_off], [-dpad_off, 0], [dpad_off, 0]])
        translate([dpad[0] + o[0], dpad[1] + o[1], -1]) cylinder(d = big_d, h = front + 2);
    for (p = [a_btn, b_btn])
        translate([p[0], p[1], -1]) cylinder(d = big_d, h = front + 2);
    for (p = [select_btn, start_btn])
        translate([p[0], p[1], -1]) cylinder(d = small_d, h = front + 2);
}

module side_cuts() {
    // USB-C, top wall
    translate([bx0 + usb_x - usb_w / 2, H - wall - 1, usb_z - usb_h / 2])
        cube([usb_w, wall + 2, usb_h]);
    // slide switch, left wall
    translate([-1, sw_y - sw_size[0] / 2, sw_z - sw_size[1] / 2])
        cube([wall + 2, sw_size[0], sw_size[1]]);
    // SD card, right wall
    if (sd_enable)
        translate([W - wall - 1, sd_y - sd_size[0] / 2, sd_z - sd_size[1] / 2])
            cube([wall + 2, sd_size[0], sd_size[1]]);
}

module shell() {
    shell_h = D - lid_t;
    difference() {
        union() {
            difference() {
                rbox(W, H, shell_h, R);
                translate([wall, wall, front]) rbox(W - 2 * wall, H - 2 * wall, shell_h, R - wall);
            }
            // board standoffs (LCD rests on the front plate, PCB on these)
            for (p = board_holes)
                translate([bx0 + p[0], by0 + p[1], front - 0.01])
                    cylinder(d = boss_d, h = lcd_stack + 0.01);
            // button perfboard standoffs
            for (p = btn_pcb_holes)
                translate([p[0], p[1], front - 0.01]) cylinder(d = 6, h = btn_pcb_z - front + 0.01);
            // lid screw posts
            for (p = lid_posts)
                translate([p[0], p[1], front - 0.01]) cylinder(d = post_d, h = shell_h - front + 0.01);
        }
        window();
        button_holes();
        side_cuts();
        // M3 self-tapping pilot holes, 1.5 mm into the front plate
        for (p = board_holes)
            translate([bx0 + p[0], by0 + p[1], front - 1.5]) cylinder(d = 2.5, h = 20);
        for (p = btn_pcb_holes)
            translate([p[0], p[1], front - 1.5]) cylinder(d = 2.5, h = 20);
        // heat-set insert holes at the top of the lid posts
        for (p = lid_posts)
            translate([p[0], p[1], shell_h - insert_len]) cylinder(d = insert_d, h = insert_len + 1);
    }
}

module lid() {
    lip_h = 1.5;  lip_t = 1.2;  clr = 0.25;
    difference() {
        union() {
            rbox(W, H, lid_t, R);
            // locating lip that drops inside the shell walls
            translate([wall + clr, wall + clr, lid_t])
                difference() {
                    rbox(W - 2 * (wall + clr), H - 2 * (wall + clr), lip_h, R - wall);
                    translate([lip_t, lip_t, -1])
                        rbox(W - 2 * (wall + clr + lip_t), H - 2 * (wall + clr + lip_t), lip_h + 2,
                             max(R - wall - lip_t, 0.5));
                }
            // battery pocket ribs (battery sits behind the display board)
            translate([W / 2 - bat[0] / 2 - 1.2, bat_y - bat[1] / 2 - 1.2, lid_t])
                difference() {
                    cube([bat[0] + 2.4, bat[1] + 2.4, bat_rib_h]);
                    translate([1.2, 1.2, -1]) cube([bat[0], bat[1], bat_rib_h + 2]);
                    translate([bat[0] / 2 + 1.2 - 6, bat[1] + 1, -1]) cube([12, 3, bat_rib_h + 2]); // wire exit
                }
        }
        // clear the lip around the screw posts
        for (p = lid_posts)
            translate([p[0], p[1], lid_t]) cylinder(d = post_d + 1, h = lip_h + 1);
        // countersunk M3 screw holes
        for (p = lid_posts) {
            translate([p[0], p[1], -1]) cylinder(d = 3.4, h = lid_t + 2);
            translate([p[0], p[1], -0.01]) cylinder(d1 = 6.5, d2 = 3.4, h = 1.6);
        }
        // speaker grille (behind the button area)
        for (i = [-4 : 4], j = [-4 : 4]) {
            x = i * 2.6;  y = j * 2.6;
            if (x * x + y * y <= spk_r * spk_r)
                translate([W / 2 + x, spk_y + y, -1]) cylinder(d = 1.8, h = lid_t + 2, $fn = 12);
        }
    }
}

// Thin slice of the front plate only: quick print to check the window and buttons.
module test_front() {
    intersection() {
        shell();
        translate([-1, -1, -1]) cube([W + 2, H + 2, front + 1 + 1]);
    }
}

if (part == "shell")           shell();
else if (part == "lid")        lid();
else if (part == "test_front") test_front();
else { shell(); translate([W + 10, 0, 0]) lid(); }
