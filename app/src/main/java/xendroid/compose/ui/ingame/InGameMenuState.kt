package xendroid.compose.ui.ingame

import androidx.annotation.StringRes
import xendroid.compose.R

/**
 * Round 2: a rail of five categories. Image = the picture and the screen it goes to; Performance =
 * frame rate and power; HUD = the performance overlay; Controls = touch, controllers and gyro;
 * Session = sound, the menu itself, this game's changes, logs and leaving.
 */
enum class InGamePage { GRAPHICS, SYSTEM, HUD, CONTROLS, SESSION }

/** How a row reads, and what a controller's ←→ and A do on it. */
enum class RowKind {
    /** A few choices side by side (pills): ←→ the one beside, A the next. */
    CHOICE,
    /** Several on/off choices side by side (check chips): ←→ move between them, A flips one. */
    MULTI,
    /** One value of a longer list (‹ value ›): ←→ the previous or the next, A the next. */
    CYCLE,
    /** On or off: A flips it, → turns it on, ← off. */
    TOGGLE,
    /** A number on a track: ←→ one step; a finger drags it. */
    SLIDER,
    /** Opens or runs something: A. */
    BUTTON,
    /** Says something; nothing to change. */
    INFO,
}

enum class InGameAction(val kind: RowKind) {
    // Image
    DISPLAY_MODE(RowKind.CHOICE), SCALING_EFFECT(RowKind.CYCLE), ANTIALIASING(RowKind.CYCLE), SHARPNESS(RowKind.CYCLE),
    DITHER(RowKind.CYCLE), COLOR_FILTER(RowKind.CYCLE),
    /** Stretch the picture to the whole screen from the next start (the core's fullscreen stretch). */
    STRETCH(RowKind.TOGGLE),
    EXTERNAL_DISPLAY(RowKind.BUTTON),
    /** 15h: a margin around the picture on a TV that cuts the edges (overscan). */
    TV_MARGIN(RowKind.CYCLE),
    /** U01: the driver the game runs on, against the one selected (read-only). */
    DRIVER_INFO(RowKind.INFO),
    ZEROFG(RowKind.CHOICE),
    WINFG(RowKind.TOGGLE), WINFG_PRESET(RowKind.CYCLE), LSFG(RowKind.TOGGLE),
    LSFG_MULTIPLIER(RowKind.CYCLE),
    /** 15d: LSFG by an output target (60/90/120/the display) instead of a multiplier by hand. */
    LSFG_TARGET(RowKind.CYCLE),
    IMPORT_LSFG_DLL(RowKind.BUTTON), CLEAR_LSFG_CACHE(RowKind.BUTTON),

    // Performance
    FPS_LIMIT(RowKind.CHOICE), REFRESH_RATE(RowKind.CYCLE), SUSTAINED_PERFORMANCE(RowKind.TOGGLE),
    PERFORMANCE_HINTS(RowKind.TOGGLE), BACKGROUND_POLICY(RowKind.CYCLE),
    /** GPU options switched live ([xendroid.compose.core.GpuLiveOption]): shaders that never stall
     *  the frame, 4x MSAA kept as 2x, cut-out transparency and the shading rate. */
    SMOOTH_SHADERS(RowKind.TOGGLE), MSAA_4X_AS_2X(RowKind.TOGGLE), CUTOUT_TRANSPARENCY(RowKind.TOGGLE),
    SHADING_RATE(RowKind.CYCLE),

    // HUD
    PERFORMANCE_HUD(RowKind.TOGGLE),
    /** Round 2: the HUD as a box beside the game (vertical) or a bar along an edge (horizontal). */
    HUD_LAYOUT(RowKind.CHOICE),
    /** 15n: FPS only, the chosen metrics, or the performance panel. */
    HUD_STYLE(RowKind.CHOICE),
    /** The metrics the HUD shows, and the FPS graph (check chips). */
    HUD_METRICS(RowKind.MULTI),
    /** Round 2: the horizontal bar along the top or the bottom. */
    HUD_POSITION(RowKind.CHOICE),
    /** 15g: the HUD's look: a box, an outline around the letters, or plain text. */
    HUD_LOOK(RowKind.CHOICE),
    HUD_SIZE(RowKind.SLIDER), HUD_OPACITY(RowKind.SLIDER),
    /** Round 2: how strongly the labels and the graph are coloured (0 = white). */
    HUD_COLORS(RowKind.SLIDER),

    // Controls
    TOUCH_CONTROLS(RowKind.TOGGLE),
    /** Round 2: the touch controls' look: modern (dark glass) or classic (coloured buttons). */
    CONTROL_STYLE(RowKind.CHOICE),
    ADAPTIVE_STICKS(RowKind.TOGGLE),
    /** U07: the free right side of the screen as a touchpad for the right stick. */
    TOUCH_CAMERA(RowKind.TOGGLE),
    EDIT_TOUCH_LAYOUT(RowKind.BUTTON),
    /** 15c: the game above and the touch controls clear of it: off, at a fold, always. */
    SPLIT_SCREEN(RowKind.CYCLE),
    CONTROLLER_RUMBLE(RowKind.CYCLE), PHONE_CONTROLLERS(RowKind.BUTTON),
    /** Controller input as it arrives instead of batched to the next frame (an A/B switch). */
    UNBUFFERED_INPUT(RowKind.TOGGLE),
    GYRO_CAMERA(RowKind.TOGGLE),
    /** The gyroscope aims always, or only while LT or LB is held. */
    GYRO_AIM(RowKind.CYCLE),
    GYRO_SENSITIVITY(RowKind.CYCLE), GYRO_CALIBRATE(RowKind.BUTTON),

    // Session
    VOLUME(RowKind.SLIDER), MUTE(RowKind.TOGGLE),
    /** Round 2: whether opening the menu pauses the game (every game). */
    PAUSE_ON_OPEN(RowKind.TOGGLE),
    /** Round 2: changes made here are kept for this game as they happen. */
    AUTO_SAVE(RowKind.TOGGLE),
    /** Round 2: this session's changes undone (the game's config as it was when it started). */
    UNDO_SESSION(RowKind.BUTTON),
    /** Round 2: this session's changes for every game (the global config; the game's own copy goes). */
    MAKE_GLOBAL(RowKind.BUTTON),
    /** C07: a numbered scene marker in the run's timeline, to line A/B runs up. */
    MARK_SCENE(RowKind.BUTTON),
    SHARE_LOGS(RowKind.BUTTON), RESUME(RowKind.BUTTON), QUIT(RowKind.BUTTON),
    /** The game's picture as it is on screen, without the menu or the HUD, saved to Pictures. */
    SCREENSHOT(RowKind.BUTTON),
    /** The next frame's GPU commands to a trace (.xtr), for Xenia Edge's trace viewer; only in
     *  the diagnostic build ([InGameMenuState.frameTrace]). */
    CAPTURE_FRAME(RowKind.BUTTON),

    /** U01: shows or hides the category's advanced options (it stays where it is in the list). */
    MORE_OPTIONS(RowKind.BUTTON),
}

/** A titled group of rows within a category; [title] null = no heading. */
class MenuGroup(@StringRes val title: Int?, val actions: List<InGameAction>)

/** Every option once, in the category and group it belongs to, most used first. */
val inGamePageGroups: Map<InGamePage, List<MenuGroup>> = mapOf(
    InGamePage.GRAPHICS to listOf(
        MenuGroup(R.string.menu_group_picture, listOf(InGameAction.DISPLAY_MODE, InGameAction.SCALING_EFFECT,
            InGameAction.ANTIALIASING, InGameAction.SHARPNESS, InGameAction.DITHER, InGameAction.COLOR_FILTER, InGameAction.STRETCH)),
        MenuGroup(R.string.menu_group_tv, listOf(InGameAction.EXTERNAL_DISPLAY, InGameAction.TV_MARGIN)),
        MenuGroup(R.string.menu_group_frame_generation, listOf(InGameAction.ZEROFG, InGameAction.WINFG, InGameAction.WINFG_PRESET, InGameAction.LSFG,
            InGameAction.LSFG_MULTIPLIER, InGameAction.LSFG_TARGET, InGameAction.IMPORT_LSFG_DLL, InGameAction.CLEAR_LSFG_CACHE)),
        MenuGroup(R.string.menu_group_driver, listOf(InGameAction.DRIVER_INFO)),
    ),
    InGamePage.SYSTEM to listOf(
        MenuGroup(R.string.menu_group_frames, listOf(InGameAction.FPS_LIMIT, InGameAction.REFRESH_RATE)),
        MenuGroup(R.string.menu_group_gpu, listOf(InGameAction.SMOOTH_SHADERS, InGameAction.MSAA_4X_AS_2X,
            InGameAction.CUTOUT_TRANSPARENCY, InGameAction.SHADING_RATE)),
        MenuGroup(R.string.menu_group_power, listOf(InGameAction.SUSTAINED_PERFORMANCE, InGameAction.PERFORMANCE_HINTS,
            InGameAction.BACKGROUND_POLICY)),
    ),
    InGamePage.HUD to listOf(
        MenuGroup(null, listOf(InGameAction.PERFORMANCE_HUD, InGameAction.HUD_LAYOUT, InGameAction.HUD_STYLE, InGameAction.HUD_METRICS)),
        MenuGroup(R.string.menu_group_hud_look, listOf(InGameAction.HUD_POSITION, InGameAction.HUD_LOOK, InGameAction.HUD_SIZE,
            InGameAction.HUD_OPACITY, InGameAction.HUD_COLORS)),
    ),
    InGamePage.CONTROLS to listOf(
        MenuGroup(R.string.menu_group_touch, listOf(InGameAction.TOUCH_CONTROLS, InGameAction.CONTROL_STYLE, InGameAction.ADAPTIVE_STICKS,
            InGameAction.TOUCH_CAMERA, InGameAction.EDIT_TOUCH_LAYOUT, InGameAction.SPLIT_SCREEN)),
        MenuGroup(R.string.menu_group_controllers, listOf(InGameAction.CONTROLLER_RUMBLE, InGameAction.PHONE_CONTROLLERS,
            InGameAction.UNBUFFERED_INPUT)),
        MenuGroup(R.string.menu_group_gyro, listOf(InGameAction.GYRO_CAMERA, InGameAction.GYRO_AIM, InGameAction.GYRO_SENSITIVITY,
            InGameAction.GYRO_CALIBRATE)),
    ),
    InGamePage.SESSION to listOf(
        MenuGroup(R.string.menu_group_sound, listOf(InGameAction.VOLUME, InGameAction.MUTE)),
        MenuGroup(R.string.menu_group_game_changes, listOf(InGameAction.AUTO_SAVE, InGameAction.UNDO_SESSION, InGameAction.MAKE_GLOBAL)),
        MenuGroup(R.string.menu_group_menu, listOf(InGameAction.PAUSE_ON_OPEN)),
        MenuGroup(R.string.menu_group_session, listOf(InGameAction.SCREENSHOT, InGameAction.CAPTURE_FRAME,
            InGameAction.MARK_SCENE, InGameAction.SHARE_LOGS,
            InGameAction.RESUME, InGameAction.QUIT)),
    ),
)

/** Every option once, on the category it belongs to (U01), in the groups' order. */
val inGamePageActions: Map<InGamePage, List<InGameAction>> = inGamePageGroups.mapValues { (_, groups) -> groups.flatMap { it.actions } }

/** U01: behind "More options" on their category: fine tuning, imports, power and experiments. */
val advancedActions: Set<InGameAction> = setOf(
    InGameAction.DITHER, InGameAction.COLOR_FILTER, InGameAction.STRETCH,
    InGameAction.LSFG_MULTIPLIER, InGameAction.LSFG_TARGET, InGameAction.IMPORT_LSFG_DLL, InGameAction.CLEAR_LSFG_CACHE,
    InGameAction.SUSTAINED_PERFORMANCE, InGameAction.PERFORMANCE_HINTS, InGameAction.BACKGROUND_POLICY,
    InGameAction.GYRO_CALIBRATE, InGameAction.UNBUFFERED_INPUT, InGameAction.MARK_SCENE,
)

/** L02: hidden in Player mode (engine internals). Frame generation is not among them: it is in
 *  every build and mode, off until the player turns it on. */
val developerActions: Set<InGameAction> = setOf(
    InGameAction.PERFORMANCE_HINTS, InGameAction.BACKGROUND_POLICY, InGameAction.SUSTAINED_PERFORMANCE,
    InGameAction.UNBUFFERED_INPUT,
)

/** The group [action] sits in on [page]; null when it is not on that page (More options). */
fun groupOf(page: InGamePage, action: InGameAction): MenuGroup? =
    inGamePageGroups.getValue(page).firstOrNull { action in it.actions }

/** Immutable navigation state shared by touch, Back, hardware buttons and hat axes. */
data class InGameMenuState(
    val open: Boolean = false,
    val page: InGamePage = InGamePage.GRAPHICS,
    val selections: List<Int> = List(InGamePage.entries.size) { 0 },
    val confirmingQuit: Boolean = false,
    val confirmationSelection: Int = 0,
    val pausedByMenu: Boolean = false,
    val logPicker: Boolean = false,
    val logCount: Int = 0,
    val logSelection: Int = 0,
    /** Developer interface (L02); Player hides [developerActions]. */
    val developer: Boolean = true,
    /** The core has the GPU trace writer (the diagnostic build); otherwise no [InGameAction.CAPTURE_FRAME]. */
    val frameTrace: Boolean = false,
    /** U01: categories whose "More options" are open. */
    val advanced: Set<InGamePage> = emptySet(),
    /** Round 2: the chip under the controller's cursor in a [RowKind.MULTI] row. */
    val chip: Int = 0,
) {
    /** The category's options as shown, what hardware navigation moves through: the common ones in
     *  their groups, then [InGameAction.MORE_OPTIONS] when the category has advanced ones, then
     *  those (still in their groups) when open. */
    fun actions(page: InGamePage = this.page): List<InGameAction> {
        val shown = inGamePageActions.getValue(page).filter(::isShown)
        val (more, common) = shown.partition { it in advancedActions }
        if (more.isEmpty()) return common
        return common + InGameAction.MORE_OPTIONS + if (page in advanced) more else emptyList()
    }

    /** How many advanced options the category holds (for the "More options" label). */
    fun advancedCount(page: InGamePage = this.page): Int =
        inGamePageActions.getValue(page).count { it in advancedActions && isShown(it) }

    private fun isShown(action: InGameAction): Boolean =
        (developer || action !in developerActions) && (frameTrace || action != InGameAction.CAPTURE_FRAME)

    /** Opens or closes the category's advanced options; the toggle keeps its place and selection. */
    fun toggleAdvanced(): InGameMenuState {
        val next = copy(advanced = if (page in advanced) advanced - page else advanced + page)
        return next.select(next.actions().indexOf(InGameAction.MORE_OPTIONS).coerceAtLeast(0))
    }

    val selected: Int
        get() = if (confirmingQuit) confirmationSelection else if (logPicker) logSelection else selections[page.ordinal]

    val count: Int
        get() = if (confirmingQuit) 2 else if (logPicker) logCount + 2 else actions().size

    val action: InGameAction?
        get() = if (confirmingQuit || logPicker) null else actions().getOrNull(selected)

    fun show(pause: Boolean): InGameMenuState =
        copy(open = true, confirmingQuit = false, logPicker = false, pausedByMenu = pause)

    fun hide(): InGameMenuState = copy(open = false, confirmingQuit = false, logPicker = false, pausedByMenu = false)

    fun changePage(delta: Int): InGameMenuState {
        if (confirmingQuit || logPicker) return this
        val pages = InGamePage.entries
        return copy(page = pages[(page.ordinal + delta + pages.size) % pages.size], chip = 0)
    }

    fun select(index: Int): InGameMenuState {
        if (confirmingQuit) return copy(confirmationSelection = index.coerceIn(0, 1))
        if (logPicker) return copy(logSelection = index.coerceIn(0, logCount + 1))
        val next = selections.toMutableList()
        next[page.ordinal] = index.coerceIn(0, (count - 1).coerceAtLeast(0))
        return copy(selections = next, chip = if (next[page.ordinal] == selected) chip else 0)
    }

    fun move(delta: Int): InGameMenuState =
        select((selected + delta % count + count) % count)

    /** A [RowKind.MULTI] row: the controller's cursor to the chip beside ([size] chips in the row). */
    fun moveChip(delta: Int, size: Int): InGameMenuState =
        if (size <= 0) this else copy(chip = (chip + delta).coerceIn(0, size - 1))

    fun askToQuit(): InGameMenuState = copy(confirmingQuit = true, confirmationSelection = 0)

    fun cancelQuit(): InGameMenuState = copy(confirmingQuit = false)
    fun showLogs(count: Int): InGameMenuState = copy(logPicker = true, logCount = count, logSelection = 0)
    fun closeLogs(): InGameMenuState = copy(logPicker = false)
}
