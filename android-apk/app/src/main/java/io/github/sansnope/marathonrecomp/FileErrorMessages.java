package io.github.sansnope.marathonrecomp;

// Ported from UnleashedRecomp-Android 0.6.1 (org.libsdl.app.FileErrorMessages).

import android.content.Context;

final class FileErrorMessages {
    static String describe(Context context, Exception exception) {
        if (exception instanceof LocalizedIOException) {
            LocalizedIOException localized = (LocalizedIOException) exception;
            int resource = context.getResources().getIdentifier(localized.resource, "string", context.getPackageName());
            if (resource != 0) return context.getString(resource, localized.detail);
        }
        String message = exception.getMessage();
        return message != null ? message : exception.getClass().getSimpleName();
    }
}
