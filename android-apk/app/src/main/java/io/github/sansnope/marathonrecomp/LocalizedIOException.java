package io.github.sansnope.marathonrecomp;

// Ported from UnleashedRecomp-Android 0.6.1 (org.libsdl.app.LocalizedIOException).

import java.io.IOException;

/** Carries a resource key through Java-only file helpers without depending on Android. */
final class LocalizedIOException extends IOException {
    final String resource;
    final String detail;

    LocalizedIOException(String resource, Object detail) {
        super(resource + (detail == null ? "" : ": " + detail));
        this.resource = resource;
        this.detail = detail == null ? "" : detail.toString();
    }
}
