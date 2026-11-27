# MarathonRecomp-Android v0.3.5.1 Hotfix

This hotfix fixes a frame-rate regression at the 60 FPS cap. On the POCO F5, v0.3.5 requested a 60 Hz display mode and the Vulkan swapchain spent longer waiting for an image. The hotfix again requests the fastest mode at the current resolution for caps up to 60 FPS. The in-game FPS cap still applies.

The tester confirmed the fix on POCO F5. At the 60 FPS cap, the test build requested 120 Hz and ran at 58.5–60 FPS in the first ten seconds of gameplay after loading the same scene. The corresponding v0.3.5 log showed 40.9–59.3 FPS. The native libraries are identical to v0.3.5.

[Changes in v0.3.5](https://github.com/SansNope/MarathonRecomp-Android/releases/tag/v0.3.5) · [Full source diff](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.5...v0.3.5.1) · [Report an issue](https://github.com/SansNope/MarathonRecomp-Android/issues)

---

## Русский

Хотфикс устраняет просадку при лимите 60 FPS. На POCO F5 версия v0.3.5 запрашивала режим экрана 60 Гц, и Vulkan дольше ждал свободное изображение для следующего кадра. При лимите до 60 FPS игра снова запрашивает самый быстрый режим экрана в текущем разрешении. Игровой лимит FPS продолжает действовать.

Исправление подтверждено на POCO F5. При лимите 60 FPS тестовая сборка запросила 120 Гц и показала 58,5–60 FPS в первые десять секунд игры после загрузки той же сцены. В сопоставимом логе v0.3.5 было 40,9–59,3 FPS. Нативные библиотеки совпадают с v0.3.5.

[Изменения v0.3.5](https://github.com/SansNope/MarathonRecomp-Android/releases/tag/v0.3.5) · [Полный список изменений](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.5...v0.3.5.1) · [Сообщить о проблеме](https://github.com/SansNope/MarathonRecomp-Android/issues)
