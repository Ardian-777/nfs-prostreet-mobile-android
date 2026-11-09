package xendroid.compose.core

import android.app.Activity
import android.app.Presentation
import android.hardware.display.DisplayManager
import android.os.Handler
import android.os.Looper
import android.view.Display
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.WindowManager
import android.graphics.PixelFormat

/** Moves the ONE guest output surface; controls and menu stay on the handset.
 * Hot-unplug returns to the primary holder without rebooting the native core. */
class ExternalGameDisplay(
    private val activity: Activity,
    private val primary: SurfaceView,
    private val callback: SurfaceHolder.Callback,
    private val detach: () -> Unit,
    private val statusChanged: (String) -> Unit,
) : DisplayManager.DisplayListener, AutoCloseable {
    private val manager = activity.getSystemService(DisplayManager::class.java)
    private var presentation: Presentation? = null
    /** 15h: the TV margin (overscan) in percent per side; the frame holding the TV's surface. */
    private var marginPercent = 0f
    private var frame: OverscanFrame? = null

    /** 15h: insets the picture on the TV by [percent] of the screen per side (now and for the next TV). */
    fun setMargin(percent: Float) {
        marginPercent = percent
        frame?.percent = percent
    }
    private var output: SurfaceView = primary
    private var closing = false
    private var detached = false
    val activeDisplay: Display? get() = presentation?.display
    /** The one output that currently belongs to the native presenter, phone or TV. */
    val outputSurfaceView: SurfaceView get() = output
    val outputWindow: android.view.Window get() = presentation?.window ?: activity.window
    fun owns(holder: SurfaceHolder) = holder === output.holder
    init { manager.registerDisplayListener(this, Handler(Looper.getMainLooper())) }

    fun choices(): List<Display> = manager.getDisplays(DisplayManager.DISPLAY_CATEGORY_PRESENTATION)
        .filter { it.isValid && it.displayId != Display.DEFAULT_DISPLAY }
    fun cycle() {
        val displays = choices()
        if (displays.isEmpty()) { statusChanged(activity.getString(xendroid.compose.R.string.tv_none)); return }
        val index = displays.indexOfFirst { it.displayId == activeDisplay?.displayId }
        if (index + 1 >= displays.size) toPhone() else toDisplay(displays[index + 1])
    }
    private fun toDisplay(display: Display) {
        val next = try { Presentation(activity, display) } catch (e: RuntimeException) {
            statusChanged(activity.getString(xendroid.compose.R.string.tv_unavailable)); return
        }
        val surface = SurfaceView(next.context)
        val holder = OverscanFrame(next.context).apply {
            setBackgroundColor(android.graphics.Color.BLACK)
            addView(surface, android.widget.FrameLayout.LayoutParams(
                android.view.ViewGroup.LayoutParams.MATCH_PARENT, android.view.ViewGroup.LayoutParams.MATCH_PARENT))
            percent = marginPercent
        }
        detach()
        detached = true
        val old = presentation
        presentation = null
        output = surface
        next.setContentView(holder)
        frame = holder
        next.window?.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON or WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE)
        // The TV's low-latency game mode (ALLM over HDMI), where the display supports it.
        if (android.os.Build.VERSION.SDK_INT >= 30) next.window?.setPreferMinimalPostProcessing(true)
        surface.holder.addCallback(callback)
        presentation = next
        old?.dismiss()
        next.setOnDismissListener { if (!closing && presentation === next) toPhone() }
        try { next.show(); detached = false; statusChanged(activity.getString(xendroid.compose.R.string.tv_on, display.name, display.refreshRate.toString())) }
        catch (e: WindowManager.InvalidDisplayException) { toPhone() }
    }
    fun toPhone() {
        if (output === primary && presentation == null && !detached) return
        detach()
        val old = presentation
        presentation = null; output = primary; frame = null
        old?.dismiss()
        if (primary.holder.surface.isValid) {
            callback.surfaceCreated(primary.holder)
            callback.surfaceChanged(primary.holder, PixelFormat.RGBA_8888, primary.width, primary.height)
        }
        detached = false
        statusChanged(activity.getString(xendroid.compose.R.string.tv_phone))
    }
    override fun onDisplayRemoved(id: Int) { if (activeDisplay?.displayId == id) toPhone() }
    override fun onDisplayAdded(id: Int) { if (activeDisplay == null) statusChanged(choices().size.let { activity.resources.getQuantityString(xendroid.compose.R.plurals.tv_available, it, it) }) }
    override fun onDisplayChanged(id: Int) {
        if (activeDisplay?.displayId == id && activeDisplay?.isValid == false) toPhone()
    }
    override fun close() {
        closing = true; manager.unregisterDisplayListener(this)
        presentation?.dismiss(); presentation = null
    }
}

/** 15h: the TV's surface inside a black frame inset by [percent] of the screen per side. */
private class OverscanFrame(context: android.content.Context) : android.widget.FrameLayout(context) {
    var percent: Float = 0f
        set(value) {
            field = value
            inset(width, height)
        }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        inset(w, h)
    }

    private fun inset(w: Int, h: Int) {
        val (x, y) = TvMargin.padding(w, h, percent)
        // Posted: padding changed inside a layout pass would be ignored until the next one.
        if (paddingLeft != x || paddingTop != y) post { setPadding(x, y, x, y) }
    }
}
