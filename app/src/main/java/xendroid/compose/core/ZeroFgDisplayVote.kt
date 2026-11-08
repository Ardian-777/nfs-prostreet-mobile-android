package xendroid.compose.core

import android.os.Build
import android.view.Surface
import android.view.SurfaceView
import android.view.View
import android.view.Window

/** The normal XenDroid ZeroFG display vote, held only while device B owns the output. */
class ZeroFgDisplayVote {
    private var votedView: SurfaceView? = null
    private var votedWindow: Window? = null
    private var votedGeneration = -1
    private var previousMode = 0

    fun update(active: Boolean, view: SurfaceView?, window: Window?, generation: Int) {
        if (!active || view == null || window == null || !view.holder.surface.isValid) {
            release()
            return
        }
        if (view === votedView && window === votedWindow && generation == votedGeneration) return
        release()
        val display = view.display ?: return
        val current = display.mode
        val modes = display.supportedModes.filter { it.refreshRate.isFinite() && it.refreshRate > 0f }
        val sameSize = modes.filter { it.physicalWidth == current.physicalWidth && it.physicalHeight == current.physicalHeight }
        val best = (sameSize.ifEmpty { modes }).maxByOrNull { it.refreshRate } ?: return
        votedView = view
        votedWindow = window
        votedGeneration = generation
        previousMode = window.attributes.preferredDisplayModeId
        // Keep the display resolution. Android still chooses the physical refresh rate.
        runCatching { window.attributes = window.attributes.apply { preferredDisplayModeId = best.modeId } }
        voteSurface(view, best.refreshRate)
    }

    fun release() {
        val view = votedView
        val window = votedWindow
        if (view != null) voteSurface(view, 0f)
        if (window != null) runCatching {
            window.attributes = window.attributes.apply { preferredDisplayModeId = previousMode }
        }
        votedView = null
        votedWindow = null
        votedGeneration = -1
    }

    private fun voteSurface(view: SurfaceView, hz: Float) {
        if (Build.VERSION.SDK_INT >= 30 && view.holder.surface.isValid) runCatching {
            if (Build.VERSION.SDK_INT >= 31) {
                view.holder.surface.setFrameRate(hz, Surface.FRAME_RATE_COMPATIBILITY_DEFAULT, Surface.CHANGE_FRAME_RATE_ALWAYS)
            } else {
                view.holder.surface.setFrameRate(hz, Surface.FRAME_RATE_COMPATIBILITY_DEFAULT)
            }
        }
        if (Build.VERSION.SDK_INT >= 35) runCatching {
            view.setRequestedFrameRate(if (hz > 0f) View.REQUESTED_FRAME_RATE_CATEGORY_HIGH else View.REQUESTED_FRAME_RATE_CATEGORY_DEFAULT)
        }
    }
}
