package io.github.sansnope.marathonrecomp;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;

/**
 * Launcher drawables in an original style that echoes the 2006 menus: a deep blue backdrop
 * with diagonal light streaks, and angular panels with cut corners, a steel frame and a
 * chrome sheen along the top edge. Everything is drawn in code; no game assets are used.
 */
final class LauncherStyle {
    private LauncherStyle() {
    }

    /** Full-screen backdrop: vertical blue gradient, a horizon glow and soft diagonal streaks. */
    static final class Backdrop extends Drawable {
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Path path = new Path();

        @Override
        public void draw(Canvas canvas) {
            Rect bounds = getBounds();
            float width = bounds.width();
            float height = bounds.height();
            if (width <= 0 || height <= 0) return;

            paint.setShader(new LinearGradient(0, bounds.top, 0, bounds.bottom,
                new int[] { 0xFF030816, 0xFF071A45, 0xFF0B2A6B },
                new float[] { 0f, 0.62f, 1f }, Shader.TileMode.CLAMP));
            canvas.drawRect(bounds, paint);

            // Horizon glow behind the lower third.
            float horizon = bounds.top + height * 0.68f;
            paint.setShader(new LinearGradient(0, horizon - height * 0.22f, 0, horizon + height * 0.22f,
                new int[] { 0x002E7BFF, 0x332E7BFF, 0x002E7BFF },
                new float[] { 0f, 0.5f, 1f }, Shader.TileMode.CLAMP));
            canvas.drawRect(bounds.left, horizon - height * 0.22f, bounds.right, horizon + height * 0.22f, paint);

            // Diagonal light streaks, wider and fainter towards the right.
            float slant = height * 0.55f;
            float[][] streaks = {
                { 0.08f, 0.020f, 0x22 }, { 0.15f, 0.006f, 0x30 }, { 0.46f, 0.045f, 0x14 },
                { 0.58f, 0.010f, 0x26 }, { 0.83f, 0.070f, 0x10 }, { 0.93f, 0.008f, 0x22 },
            };
            for (float[] streak : streaks) {
                float x = bounds.left + width * streak[0];
                float w = width * streak[1];
                int alpha = (int) streak[2];
                paint.setShader(new LinearGradient(0, bounds.top, 0, bounds.bottom,
                    (alpha << 24) | 0x9FD8FF, 0x009FD8FF, Shader.TileMode.CLAMP));
                path.reset();
                path.moveTo(x, bounds.top);
                path.lineTo(x + w, bounds.top);
                path.lineTo(x + w - slant, bounds.bottom);
                path.lineTo(x - slant, bounds.bottom);
                path.close();
                canvas.drawPath(path, paint);
            }
            paint.setShader(null);
        }

        @Override
        public void setAlpha(int alpha) {
            paint.setAlpha(alpha);
        }

        @Override
        public void setColorFilter(ColorFilter colorFilter) {
            paint.setColorFilter(colorFilter);
        }

        @Override
        public int getOpacity() {
            return PixelFormat.OPAQUE;
        }
    }

    /**
     * Angular panel: the top-left and bottom-right corners are cut at 45 degrees. Fills with a
     * vertical gradient, draws an optional frame and a short chrome sheen along the top edge.
     */
    static final class Panel extends Drawable {
        private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint frame = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint sheen = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Path path = new Path();
        private final int topColor;
        private final int bottomColor;
        private final int frameColor;
        private final float cut;
        private final float stroke;
        private final boolean withSheen;

        Panel(int topColor, int bottomColor, int frameColor, float cut, float stroke, boolean withSheen) {
            this.topColor = topColor;
            this.bottomColor = bottomColor;
            this.frameColor = frameColor;
            this.cut = cut;
            this.stroke = stroke;
            this.withSheen = withSheen;
            frame.setStyle(Paint.Style.STROKE);
            frame.setStrokeWidth(stroke);
            frame.setColor(frameColor);
            sheen.setStyle(Paint.Style.STROKE);
            sheen.setStrokeWidth(Math.max(1f, stroke));
        }

        /** Opaque copy of the shape, for use as a ripple mask. */
        Panel mask() {
            return new Panel(0xFFFFFFFF, 0xFFFFFFFF, 0, cut, 0, false);
        }

        @Override
        protected void onBoundsChange(Rect bounds) {
            float inset = stroke / 2f;
            float left = bounds.left + inset;
            float top = bounds.top + inset;
            float right = bounds.right - inset;
            float bottom = bounds.bottom - inset;
            float c = Math.min(cut, Math.min(right - left, bottom - top) / 2f);

            path.reset();
            path.moveTo(left + c, top);
            path.lineTo(right, top);
            path.lineTo(right, bottom - c);
            path.lineTo(right - c, bottom);
            path.lineTo(left, bottom);
            path.lineTo(left, top + c);
            path.close();

            fill.setShader(new LinearGradient(0, top, 0, bottom, topColor, bottomColor, Shader.TileMode.CLAMP));
            sheen.setShader(new LinearGradient(left + c, 0, left + c + (right - left) * 0.6f, 0,
                0x66FFFFFF, 0x00FFFFFF, Shader.TileMode.CLAMP));
        }

        @Override
        public void draw(Canvas canvas) {
            canvas.drawPath(path, fill);
            if (frameColor != 0 && stroke > 0) canvas.drawPath(path, frame);
            if (withSheen) {
                Rect bounds = getBounds();
                float y = bounds.top + stroke * 1.5f + 1f;
                float c = Math.min(cut, Math.min(bounds.width(), bounds.height()) / 2f);
                canvas.drawLine(bounds.left + c + stroke, y, bounds.right - stroke, y, sheen);
            }
        }

        @Override
        public void setAlpha(int alpha) {
            fill.setAlpha(alpha);
            frame.setAlpha(alpha);
        }

        @Override
        public void setColorFilter(ColorFilter colorFilter) {
            fill.setColorFilter(colorFilter);
            frame.setColorFilter(colorFilter);
        }

        @Override
        public int getOpacity() {
            return PixelFormat.TRANSLUCENT;
        }
    }

    /** Thin line that fades out to the right, used under headings and between panes. */
    static Drawable fadeLine(int color, boolean horizontal) {
        return new GradientDrawable(
            horizontal ? GradientDrawable.Orientation.LEFT_RIGHT : GradientDrawable.Orientation.TOP_BOTTOM,
            new int[] { color, color & 0x00FFFFFF });
    }
}
