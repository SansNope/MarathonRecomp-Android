#include <user/config.h>

/*
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! LOCALISATION NOTES !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    - Ensure brand names are always presented on the same line.

      Correct:
      This is a string that contains a brand name like
      Xbox 360, which is one of the two consoles to have a port of
      SONIC THE HEDGEHOG.

      Incorrect:
      This is a string that contains a brand name like Xbox
      360, which is one of the two consoles to have a port of SONIC THE
      HEDGEHOG.

    - Ensure your locale is added in the correct order following the language enum.

      Correct:
      {
          { ELanguage::English,  "Example" },
          { ELanguage::Japanese, "Example" },
          { ELanguage::German,   "Example" },
          { ELanguage::French,   "Example" },
          { ELanguage::Spanish,  "Example" },
          { ELanguage::Italian,  "Example" }
      }

      Incorrect:
      {
          { ELanguage::English,  "Example" },
          { ELanguage::French,   "Example" },
          { ELanguage::Spanish,  "Example" },
          { ELanguage::German,   "Example" },
          { ELanguage::Italian,  "Example" },
          { ELanguage::Japanese, "Example" }
      }

    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

#define CONFIG_DEFINE_LOCALE(name) \
    CONFIG_LOCALE g_##name##_locale =

#define CONFIG_DEFINE_ENUM_LOCALE(type) \
    CONFIG_ENUM_LOCALE(type) g_##type##_locale =

CONFIG_DEFINE_LOCALE(Language)
{
    { ELanguage::English,  { "Language", "Change the language used for text." } },
    { ELanguage::Japanese, { "言語", "ゲーム内の表示言語を変更できます" } },
    { ELanguage::German,   { "Sprache", "Ändere die Textsprache." } },
    { ELanguage::French,   { "Langue", "Modifier la langue utilisée pour le texte." } },
    { ELanguage::Spanish,  { "Idioma", "Cambia el idioma utilizado para los textos." } },
    { ELanguage::Italian,  { "Lingua", "Cambia la lingua utilizzata per il testo." } }
};

// Notes: do not localise this.
CONFIG_DEFINE_ENUM_LOCALE(ELanguage)
{
    {
        ELanguage::English,
        {
            { ELanguage::English,  { "English", "" } },
            { ELanguage::Japanese, { "日本語", "" } },
            { ELanguage::German,   { "Deutsch", "" } },
            { ELanguage::French,   { "Français", "" } },
            { ELanguage::Spanish,  { "Español", "" } },
            { ELanguage::Italian,  { "Italiano", "" } }
        }
    }
};

CONFIG_DEFINE_LOCALE(VoiceLanguage)
{
    { ELanguage::English,  { "Voice Language", "Change the language used for character voices." } },
    { ELanguage::Japanese, { "音声言語", "ゲーム内の音声言語を変更できます" } },
    { ELanguage::German,   { "Stimmeinstellung", "Ändere die Sprache, die für Charakterstimmen benutzt wird." } },
    { ELanguage::French,   { "Langue de doublage", "Modifie la langue utilisée pour la voix des personnages." } },
    { ELanguage::Spanish,  { "Idioma de voz", "Cambia el idioma utilizado para las voces de los personajes." } },
    { ELanguage::Italian,  { "Lingua delle voci", "Modifica la lingua utilizzata per le voci dei personaggi." } }
};

CONFIG_DEFINE_ENUM_LOCALE(EVoiceLanguage)
{
    {
        ELanguage::English,
        {
            { EVoiceLanguage::English,  { "English", "" } },
            { EVoiceLanguage::Japanese, { "Japanese", "" } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { EVoiceLanguage::English,  { "英語", "" } },
            { EVoiceLanguage::Japanese, { "日本語", "" } }
        }
    },
    {
        ELanguage::German,
        {
            { EVoiceLanguage::English,  { "Englisch", "" } },
            { EVoiceLanguage::Japanese, { "Japanisch", "" } }
        }
    },
    {
        ELanguage::French,
        {
            { EVoiceLanguage::English,  { "Anglais", "" } },
            { EVoiceLanguage::Japanese, { "Japonais", "" } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { EVoiceLanguage::English,  { "Inglés", "" } },
            { EVoiceLanguage::Japanese, { "Japonés", "" } }
        }
    },
    {
        ELanguage::Italian,
        {
            { EVoiceLanguage::English,  { "Inglese", "" } },
            { EVoiceLanguage::Japanese, { "Giapponese", "" } }
        }
    },
};

CONFIG_DEFINE_LOCALE(Subtitles)
{
    { ELanguage::English,  { "Subtitles", "Show subtitles during dialogue." } },
    { ELanguage::Japanese, { "字幕", "字幕の表示を選択できます" } },
    { ELanguage::German,   { "Untertitel", "Zeige Untertitel bei Dialogen." } },
    { ELanguage::French,   { "Sous-titres", "Affiche les sous-titres pendant les dialogues." } },
    { ELanguage::Spanish,  { "Subtítulos", "Muestra subtítulos durante los diálogos." } },
    { ELanguage::Italian,  { "Sottotitoli", "Mostra i sottotitoli durante i dialoghi." } }
};

CONFIG_DEFINE_LOCALE(Hints)
{
    { ELanguage::English,  { "Hints", "Show hints during gameplay." } },
    { ELanguage::Japanese, { "ヒントリング", "ゲーム内にヒントリングを表示するか選択できます" } },
    { ELanguage::German,   { "Hinweise", "Zeige Hinweise während des Spiels." } },
    { ELanguage::French,   { "Indices", "Affiche les indices pendant le jeu." } },
    { ELanguage::Spanish,  { "Pistas", "Muestra pistas durante el juego." } },
    { ELanguage::Italian,  { "Indizi", "Mostra degli indizzi durante il gioco." } }
};

CONFIG_DEFINE_LOCALE(ControlTutorial)
{
    { ELanguage::English,  { "Control Tutorial", "Show controller hints during gameplay." } },
    { ELanguage::Japanese, { "アクションナビ", "ゲーム内にアクションナビを表示するか選択できます" } },
    { ELanguage::German,   { "Steuerungsanleitung", "Zeige Steuerungshinweise während des Spiels." } },
    { ELanguage::French,   { "Indication des commandes", "Affiche les indications des commandes pendant le jeu." } },
    { ELanguage::Spanish,  { "Tutorial de controles", "Muestra pistas de controles durante el juego." } },
    { ELanguage::Italian,  { "Tutorial dei comandi", "Mostra i tutorial dei comandi durante il gioco." } }
};

CONFIG_DEFINE_LOCALE(Autosave)
{
    { ELanguage::English,  { "Autosave", "Save the game automatically at manual save points." } },
    { ELanguage::Japanese, { "オートセーブ", "手動セーブポイントでゲームを自動的にセーブします" } },
    { ELanguage::German,   { "Automatisches Speichern", "Speichert das Spiel automatisch an manuellen Speicherpunkten." } },
    { ELanguage::French,   { "Sauvegarde Auto", "Sauvegarder automatiquement la partie lorsque le jeu propose une sauvegarde." } },
    { ELanguage::Spanish,  { "Autoguardado", "Guarda el juego automáticamente en los puntos de guardado manuales." } },
    { ELanguage::Italian,  { "Salvataggio automatico", "Salva automaticamente il gioco nei punti di salvataggio manuali." } }
};

CONFIG_DEFINE_LOCALE(AchievementNotifications)
{
    { ELanguage::English,  { "Achievement Notifications", "Show notifications for unlocking achievements. Achievements will still be rewarded with notifications disabled." } },
    { ELanguage::Japanese, { "実績通知", "実績通知の有無を選択できます オフにしても実績は付与されます" } },
    { ELanguage::German,   { "Erfolgsbenachrichtigungen", "Zeige Benachrichtigungen für das Freischalten von Erfolgen. Erfolge werden weiterhin freigeschaltet, auch wenn die Benachrichtigungen ausgeschaltet sind." } },
    { ELanguage::French,   { "Notification des succès", "Affiche les notifications pour le déverrouillage des succès. Les succès seront toujours obtenus même si les notifications sont désactivées." } },
    { ELanguage::Spanish,  { "Notificaciones de logros", "Muestra notificaciones al desbloquear logros. Los logros se seguirán obteniendo aunque las notificaciones estén desactivadas." } },
    { ELanguage::Italian,  { "Notifiche obiettivi", "Mostra delle notifiche quando sblocchi degli obiettivi. Gli obiettivi verranno comunque assegnati anche con le notifiche disattivate." } }
};

CONFIG_DEFINE_LOCALE(MidairControlForMachSpeed)
{
    { ELanguage::English,  { "Max Speed Air Control", "Steer Sonic in midair during Max Speed sections." } },
    { ELanguage::Japanese, { "高速ステージの空中操作", "高速ステージでソニックのジャンプ中に進行方向を変えられます" } },
    { ELanguage::German,   { "Luftsteuerung bei Max Speed", "Steuere Sonic in Max-Speed-Abschnitten auch in der Luft." } },
    { ELanguage::French,   { "Contrôle aérien à pleine vitesse", "Dirige Sonic en plein saut dans les sections à pleine vitesse." } },
    { ELanguage::Spanish,  { "Control aéreo a máxima velocidad", "Controla a Sonic en el aire durante los tramos de máxima velocidad." } },
    { ELanguage::Italian,  { "Controllo aereo ad alta velocità", "Controlla Sonic in aria nelle sezioni ad alta velocità." } }
};

CONFIG_DEFINE_LOCALE(DisableClimbFromJump)
{
    { ELanguage::English,  { "Wall Jump Fix", "Stops Knuckles and Rouge from grabbing walls during a normal jump. They can still grab walls while gliding." } },
    { ELanguage::Japanese, { "壁ジャンプの修正", "ナックルズとルージュが通常ジャンプ中に壁につかまるのを防ぎます。滑空中は壁につかまれます" } },
    { ELanguage::German,   { "Wandabsprung-Korrektur", "Verhindert, dass sich Knuckles und Rouge bei einem normalen Sprung an Wänden festhalten. Beim Gleiten ist das weiterhin möglich." } },
    { ELanguage::French,   { "Correction du saut mural", "Empêche Knuckles et Rouge de s'accrocher aux murs lors d'un saut normal. Ils peuvent encore s'y accrocher en planant." } },
    { ELanguage::Spanish,  { "Corrección del salto de pared", "Evita que Knuckles y Rouge se agarren a las paredes durante un salto normal. Todavía pueden agarrarse al planear." } },
    { ELanguage::Italian,  { "Correzione del salto a parete", "Impedisce a Knuckles e Rouge di aggrapparsi alle pareti durante un salto normale. Possono ancora farlo in planata." } }
};

CONFIG_DEFINE_LOCALE(MidairControlForSnowboards)
{
    { ELanguage::English,  { "Midair Control for Snowboards", "Steer the snowboard while airborne." } },
    { ELanguage::Japanese, { "スノーボードの空中操作", "ジャンプ中にスノーボードの進行方向を変えられます" } },
    { ELanguage::German,   { "Snowboard-Luftsteuerung", "Steuere das Snowboard auch in der Luft." } },
    { ELanguage::French,   { "Contrôle aérien du snowboard", "Dirige le snowboard pendant un saut." } },
    { ELanguage::Spanish,  { "Control aéreo del snowboard", "Controla la tabla de snowboard en el aire." } },
    { ELanguage::Italian,  { "Controllo aereo dello snowboard", "Sterza con lo snowboard mentre sei in aria." } }
};

CONFIG_DEFINE_LOCALE(DisablePushState)
{
    { ELanguage::English,  { "Wall Push Fix", "Prevents characters from getting stuck while pushing against walls." } },
    { ELanguage::Japanese, { "壁際での引っかかり修正", "壁を押したときに身動きが取れなくなるのを防ぎます" } },
    { ELanguage::German,   { "Korrektur beim Drücken gegen Wände", "Verhindert, dass Figuren beim Drücken gegen Wände hängen bleiben." } },
    { ELanguage::French,   { "Correction du blocage contre les murs", "Empêche les personnages de rester bloqués en poussant contre un mur." } },
    { ELanguage::Spanish,  { "Corrección de bloqueo en paredes", "Evita que los personajes se queden atascados al empujar contra una pared." } },
    { ELanguage::Italian,  { "Correzione del blocco contro le pareti", "Evita che i personaggi rimangano bloccati spingendo contro una parete." } }
};

CONFIG_DEFINE_LOCALE(RestoreSonicActionGauge)
{
    { ELanguage::English,  { "Restore Sonic's Action Gauge", "Sonic's gems draw from a gauge that can refill." } },
    { ELanguage::Japanese, { "ソニックのアクションゲージ復元", "ソニックのジェムを使うとゲージが減り、再び回復します" } },
    { ELanguage::German,   { "Sonics Aktionsanzeige wiederherstellen", "Sonics Edelsteine verbrauchen Energie aus einer Anzeige, die sich wieder füllt." } },
    { ELanguage::French,   { "Rétablir la jauge d'action de Sonic", "Les gemmes de Sonic consomment une jauge qui peut se remplir à nouveau." } },
    { ELanguage::Spanish,  { "Restaurar el indicador de acción de Sonic", "Las gemas de Sonic consumen un indicador que puede recargarse." } },
    { ELanguage::Italian,  { "Ripristina la barra azione di Sonic", "Le gemme di Sonic consumano energia da una barra che può ricaricarsi." } }
};

CONFIG_DEFINE_LOCALE(TailsGauge)
{
    { ELanguage::English,  { "Tails' Flight Gauge", "Shows Tails' remaining flight time. Re-enter the stage to apply the change." } },
    { ELanguage::Japanese, { "テイルスの飛行ゲージ", "テイルスの残り飛行時間を表示します。変更はステージに入り直すと反映されます" } },
    { ELanguage::German,   { "Tails' Fluganzeige", "Zeigt Tails' verbleibende Flugzeit. Betritt den Level erneut, damit die Änderung wirksam wird." } },
    { ELanguage::French,   { "Jauge de vol de Tails", "Affiche le temps de vol restant de Tails. Rentre dans le niveau pour appliquer le changement." } },
    { ELanguage::Spanish,  { "Indicador de vuelo de Tails", "Muestra el tiempo de vuelo restante de Tails. Vuelve a entrar en el nivel para aplicar el cambio." } },
    { ELanguage::Italian,  { "Indicatore di volo di Tails", "Mostra il tempo di volo rimanente di Tails. Entra di nuovo nel livello per applicare la modifica." } }
};

CONFIG_DEFINE_LOCALE(ControllableTornadoAttack)
{
    { ELanguage::English,  { "Controllable Tornado Attack", "Lets Sonic steer during his tornado attack. Re-enter the stage to update the sound." } },
    { ELanguage::Japanese, { "トルネードアタックの操作", "トルネードアタック中にソニックを操作できます。効果音の変更はステージに入り直すと反映されます" } },
    { ELanguage::German,   { "Steuerbarer Tornadoangriff", "Steuere Sonic während seines Tornadoangriffs. Betritt den Level erneut, um den Ton zu aktualisieren." } },
    { ELanguage::French,   { "Attaque tornade contrôlable", "Dirige Sonic pendant son attaque tornade. Rentre dans le niveau pour actualiser le son." } },
    { ELanguage::Spanish,  { "Ataque tornado controlable", "Controla a Sonic durante su ataque tornado. Vuelve a entrar en el nivel para actualizar el sonido." } },
    { ELanguage::Italian,  { "Attacco tornado controllabile", "Muovi Sonic durante il suo attacco tornado. Entra di nuovo nel livello per aggiornare l'audio." } }
};

CONFIG_DEFINE_LOCALE(ControllableBoundAttack)
{
    { ELanguage::English,  { "Steer During Bounce Attack", "Lets Sonic steer in the air during a Bounce Attack." } },
    { ELanguage::Japanese, { "バウンドアタック中の操作", "バウンドアタック中にソニックの進行方向を変えられます" } },
    { ELanguage::German,   { "Steuerung beim Aufprallangriff", "Steuere Sonic während des Aufprallangriffs in der Luft." } },
    { ELanguage::French,   { "Contrôle pendant l'attaque rebond", "Dirige Sonic en l'air pendant son attaque rebond." } },
    { ELanguage::Spanish,  { "Control durante el ataque de rebote", "Controla a Sonic en el aire durante su ataque de rebote." } },
    { ELanguage::Italian,  { "Controllo durante l'attacco rimbalzante", "Muovi Sonic in aria durante il suo attacco rimbalzante." } }
};

CONFIG_DEFINE_LOCALE(ControllableSpinkick)
{
    { ELanguage::English,  { "Steer During Spin Kick", "Lets you change direction during a Spin Kick." } },
    { ELanguage::Japanese, { "スピンキック中の操作", "スピンキック中に進行方向を変えられます" } },
    { ELanguage::German,   { "Steuerung beim Drehkick", "Ändere während des Drehkicks die Bewegungsrichtung." } },
    { ELanguage::French,   { "Contrôle pendant le coup de pied tournant", "Change de direction pendant le coup de pied tournant." } },
    { ELanguage::Spanish,  { "Control durante la patada giratoria", "Permite cambiar de dirección durante la patada giratoria." } },
    { ELanguage::Italian,  { "Controllo durante il calcio rotante", "Consente di cambiare direzione durante il calcio rotante." } }
};

CONFIG_DEFINE_LOCALE(ControllableHammerAttack)
{
    { ELanguage::English,  { "Steer During Hammer Attack", "Lets Amy move while swinging her hammer." } },
    { ELanguage::Japanese, { "ハンマー攻撃中の操作", "エミーがハンマーを振っている間も移動できます" } },
    { ELanguage::German,   { "Steuerung beim Hammerangriff", "Bewege Amy, während sie ihren Hammer schwingt." } },
    { ELanguage::French,   { "Contrôle pendant l'attaque au marteau", "Déplace Amy pendant qu'elle donne un coup de marteau." } },
    { ELanguage::Spanish,  { "Control durante el ataque con martillo", "Permite mover a Amy mientras golpea con el martillo." } },
    { ELanguage::Italian,  { "Controllo durante l'attacco col martello", "Consente di muovere Amy mentre usa il martello." } }
};

CONFIG_DEFINE_LOCALE(DisableMidairSpinningClaw)
{
    { ELanguage::English,  { "Disable Airborne Spinning Claw", "Disables Blaze's Spinning Claw in midair so it cannot interrupt a Homing Attack." } },
    { ELanguage::Japanese, { "空中のスピニングクローを無効化", "ブレイズの空中スピニングクローを無効にして、ホーミングアタックの中断を防ぎます" } },
    { ELanguage::German,   { "Drehklaue in der Luft deaktivieren", "Deaktiviert Blazes Drehklaue in der Luft, damit sie den Zielangriff nicht abbricht." } },
    { ELanguage::French,   { "Désactiver la griffe tournante en l'air", "Empêche la griffe tournante de Blaze en l'air d'interrompre une attaque téléguidée." } },
    { ELanguage::Spanish,  { "Desactivar la garra giratoria en el aire", "Desactiva la garra giratoria aérea de Blaze para que no interrumpa el ataque teledirigido." } },
    { ELanguage::Italian,  { "Disattiva l'artiglio rotante in aria", "Disattiva l'artiglio rotante aereo di Blaze per evitare che interrompa un attacco a ricerca." } }
};

CONFIG_DEFINE_LOCALE(DisableSandJump)
{
    { ELanguage::English,  { "Full Jump Height in Sand and Water", "Uses normal jump height when jumping from sand or water." } },
    { ELanguage::Japanese, { "砂や水の中でも通常のジャンプ", "砂や水からジャンプするときも通常の高さまで跳べます" } },
    { ELanguage::German,   { "Normale Sprunghöhe in Sand und Wasser", "Behält beim Absprung aus Sand oder Wasser die normale Sprunghöhe bei." } },
    { ELanguage::French,   { "Hauteur de saut normale dans le sable et l'eau", "Conserve la hauteur de saut normale en sautant depuis le sable ou l'eau." } },
    { ELanguage::Spanish,  { "Altura de salto normal en arena y agua", "Mantiene la altura normal al saltar desde la arena o el agua." } },
    { ELanguage::Italian,  { "Altezza normale del salto in sabbia e acqua", "Mantiene l'altezza normale quando salti dalla sabbia o dall'acqua." } }
};

CONFIG_DEFINE_LOCALE(UnlockSnowboardControl)
{
    { ELanguage::English,  { "Full Snowboard Steering", "Lets you steer the snowboard in any direction without charging a jump." } },
    { ELanguage::Japanese, { "スノーボードの全方向操作", "ジャンプのためをせずに、スノーボードを全方向に操作できます" } },
    { ELanguage::German,   { "Freie Snowboard-Steuerung", "Lenke das Snowboard in jede Richtung, ohne einen Sprung aufzuladen." } },
    { ELanguage::French,   { "Contrôle complet du snowboard", "Dirige le snowboard dans toutes les directions sans charger un saut." } },
    { ELanguage::Spanish,  { "Control completo de la tabla de nieve", "Permite dirigir la tabla en cualquier dirección sin cargar un salto." } },
    { ELanguage::Italian,  { "Controllo completo dello snowboard", "Consente di sterzare in ogni direzione senza caricare un salto." } }
};

CONFIG_DEFINE_LOCALE(InfiniteLives)
{
    { ELanguage::English,  { "Keep Lives on Death", "Prevents a normal death from taking a life." } },
    { ELanguage::Japanese, { "ミスしても残機を維持", "通常のミスで残機が減らなくなります" } },
    { ELanguage::German,   { "Leben beim Tod behalten", "Bei einem normalen Tod wird kein Leben abgezogen." } },
    { ELanguage::French,   { "Conserver ses vies", "Empêche la perte d'une vie lors d'une mort normale." } },
    { ELanguage::Spanish,  { "Conservar vidas al morir", "Evita perder una vida al morir de forma normal." } },
    { ELanguage::Italian,  { "Mantieni le vite alla morte", "Evita di perdere una vita in caso di morte normale." } }
};

CONFIG_DEFINE_LOCALE(RestorePauseMissionText)
{
    { ELanguage::English,  { "Show Mission Text on Pause", "Shows the mission objective in the pause menu when one is available." } },
    { ELanguage::Japanese, { "ポーズ中にミッション目標を表示", "ミッション目標がある場合にポーズメニューに表示します" } },
    { ELanguage::German,   { "Missionsziel im Pausenmenü", "Zeigt das Missionsziel im Pausenmenü an, sofern eines vorhanden ist." } },
    { ELanguage::French,   { "Objectif de mission en pause", "Affiche l'objectif de mission dans le menu de pause s'il est disponible." } },
    { ELanguage::Spanish,  { "Objetivo de misión en pausa", "Muestra el objetivo de misión en el menú de pausa cuando está disponible." } },
    { ELanguage::Italian,  { "Obiettivo missione in pausa", "Mostra l'obiettivo della missione nel menu di pausa quando disponibile." } }
};

CONFIG_DEFINE_LOCALE(DisableTitleInputDelay)
{
    { ELanguage::English,  { "Faster Title Screen Input", "Lets the title screen accept input sooner." } },
    { ELanguage::Japanese, { "タイトル画面の入力待ちを短縮", "タイトル画面でボタン入力を早く受け付けます" } },
    { ELanguage::German,   { "Schnellere Eingabe im Titelbildschirm", "Der Titelbildschirm reagiert früher auf Eingaben." } },
    { ELanguage::French,   { "Écran titre plus réactif", "L'écran titre accepte les commandes plus rapidement." } },
    { ELanguage::Spanish,  { "Pantalla de título más rápida", "La pantalla de título acepta las pulsaciones antes." } },
    { ELanguage::Italian,  { "Schermata del titolo più reattiva", "La schermata del titolo accetta prima i comandi." } }
};

CONFIG_DEFINE_LOCALE(RestoreChainJumpFlips)
{
    { ELanguage::English,  { "Restore Chain Jump Flips", "Restores the flipping animations between chain jumps." } },
    { ELanguage::Japanese, { "チェーンジャンプ中の宙返り", "チェーンジャンプの合間の宙返りを復元します" } },
    { ELanguage::German,   { "Saltos bei Kettensprüngen", "Stellt die Saltoanimationen zwischen Kettensprüngen wieder her." } },
    { ELanguage::French,   { "Saltos des sauts en chaîne", "Rétablit les animations de salto entre les sauts en chaîne." } },
    { ELanguage::Spanish,  { "Volteretas en saltos en cadena", "Restaura las animaciones de voltereta entre saltos en cadena." } },
    { ELanguage::Italian,  { "Capriole nei salti a catena", "Ripristina le animazioni delle capriole tra i salti a catena." } }
};

CONFIG_DEFINE_LOCALE(RestoreContextualHUDColours)
{
    { ELanguage::English,  { "Episode HUD Colours", "Matches HUD colours to the character's episode. Re-enter the level to refresh the HUD." } },
    { ELanguage::Japanese, { "エピソードに合わせたHUDの色", "キャラクターのエピソードに合わせてHUDの色を変えます。ステージに入り直すと反映されます" } },
    { ELanguage::German,   { "HUD-Farben passend zur Episode", "Passt die HUD-Farben an die Episode der Figur an. Betritt den Level erneut, um das HUD zu aktualisieren." } },
    { ELanguage::French,   { "Couleurs du HUD par épisode", "Adapte les couleurs du HUD à l'épisode du personnage. Rentre dans le niveau pour actualiser le HUD." } },
    { ELanguage::Spanish,  { "Colores del HUD según el episodio", "Adapta los colores del HUD al episodio del personaje. Vuelve a entrar en el nivel para actualizar el HUD." } },
    { ELanguage::Italian,  { "Colori HUD per episodio", "Adatta i colori dell'HUD all'episodio del personaggio. Entra di nuovo nel livello per aggiornare l'HUD." } }
};

CONFIG_DEFINE_LOCALE(DisableKingdomValleyMist)
{
    { ELanguage::English,  { "Disable Kingdom Valley Mist", "Removes the mist effect from the Max Speed section of Kingdom Valley." } },
    { ELanguage::Japanese, { "キングダムバレーの霧を無効化", "キングダムバレーの高速区間に表示される霧を消します" } },
    { ELanguage::German,   { "Nebel in Kingdom Valley deaktivieren", "Entfernt den Nebeleffekt im Max-Speed-Abschnitt von Kingdom Valley." } },
    { ELanguage::French,   { "Désactiver la brume de Kingdom Valley", "Supprime la brume de la section à pleine vitesse de Kingdom Valley." } },
    { ELanguage::Spanish,  { "Quitar la niebla de Kingdom Valley", "Elimina la niebla del tramo a máxima velocidad de Kingdom Valley." } },
    { ELanguage::Italian,  { "Disattiva la nebbia di Kingdom Valley", "Rimuove la nebbia nella sezione ad alta velocità di Kingdom Valley." } }
};

CONFIG_DEFINE_LOCALE(ControllableHomingRecovery)
{
    { ELanguage::English,  { "Steer After Homing Attack", "Lets Sonic steer during recovery after a Homing Attack." } },
    { ELanguage::Japanese, { "ホーミングアタック後の操作", "ホーミングアタック直後にソニックの進行方向を変えられます" } },
    { ELanguage::German,   { "Steuerung nach Zielangriff", "Steuere Sonic direkt nach einem Zielangriff." } },
    { ELanguage::French,   { "Contrôle après l'attaque téléguidée", "Dirige Sonic pendant la récupération après une attaque téléguidée." } },
    { ELanguage::Spanish,  { "Control tras el ataque teledirigido", "Controla a Sonic durante la recuperación tras un ataque teledirigido." } },
    { ELanguage::Italian,  { "Controllo dopo l'attacco a ricerca", "Muovi Sonic durante il recupero dopo un attacco a ricerca." } }
};

CONFIG_DEFINE_LOCALE(RestoreHomingFlips)
{
    { ELanguage::English,  { "Restore Homing Flips", "Restores Sonic's extra flips after a Homing Attack." } },
    { ELanguage::Japanese, { "ホーミング後の宙返りを復元", "ホーミングアタック直後のソニックの宙返りを復元します" } },
    { ELanguage::German,   { "Salti nach Zielangriff wiederherstellen", "Stellt Sonics zusätzliche Salti nach einem Zielangriff wieder her." } },
    { ELanguage::French,   { "Rétablir les saltos après l'attaque", "Rétablit les saltos supplémentaires de Sonic après une attaque téléguidée." } },
    { ELanguage::Spanish,  { "Restaurar volteretas tras el ataque", "Recupera las volteretas de Sonic tras un ataque teledirigido." } },
    { ELanguage::Italian,  { "Ripristina le capriole dopo l'attacco", "Ripristina le capriole aggiuntive di Sonic dopo un attacco a ricerca." } }
};

CONFIG_DEFINE_LOCALE(RestoreHomingGravity)
{
    { ELanguage::English,  { "Restore Homing Gravity", "Restores Sonic's Homing Attack gravity from early versions." } },
    { ELanguage::Japanese, { "ホーミング時の重力を復元", "開発版にあったソニックのホーミングアタック時の重力を復元します" } },
    { ELanguage::German,   { "Frühere Zielangriff-Schwerkraft", "Stellt die Schwerkraft für Sonics Zielangriff aus frühen Versionen wieder her." } },
    { ELanguage::French,   { "Gravité de l'attaque téléguidée", "Rétablit la gravité de l'attaque téléguidée de Sonic des premières versions." } },
    { ELanguage::Spanish,  { "Gravedad del ataque teledirigido", "Recupera la gravedad del ataque teledirigido de Sonic de versiones tempranas." } },
    { ELanguage::Italian,  { "Gravità dell'attacco a ricerca", "Ripristina la gravità dell'attacco a ricerca di Sonic delle prime versioni." } }
};

CONFIG_DEFINE_LOCALE(RestoreHomingSpam)
{
    { ELanguage::English,  { "Repeat Homing Attacks", "Lets Sonic and Shadow chain Homing Attacks more quickly." } },
    { ELanguage::Japanese, { "ホーミングアタックの連発", "ソニックとシャドウがホーミングアタックを素早く連発できます" } },
    { ELanguage::German,   { "Schnelle Zielangriffe", "Sonic und Shadow können Zielangriffe schneller wiederholen." } },
    { ELanguage::French,   { "Attaques téléguidées en chaîne", "Sonic et Shadow peuvent enchaîner les attaques téléguidées plus rapidement." } },
    { ELanguage::Spanish,  { "Ataques teledirigidos en cadena", "Sonic y Shadow pueden encadenar ataques teledirigidos más rápido." } },
    { ELanguage::Italian,  { "Attacchi a ricerca in sequenza", "Sonic e Shadow possono ripetere più rapidamente gli attacchi a ricerca." } }
};

CONFIG_DEFINE_LOCALE(AntigravityRetainsMomentum)
{
    { ELanguage::English,  { "Sliding Attack Retains Speed", "Keeps Sonic's current speed when starting a Sliding Attack." } },
    { ELanguage::Japanese, { "スライディング時の速度維持", "ソニックがスライディングアタックを始めるときに速度を維持します" } },
    { ELanguage::German,   { "Tempo beim Schlitterangriff", "Sonic behält sein Tempo zu Beginn des Schlitterangriffs." } },
    { ELanguage::French,   { "Vitesse conservée en glissade", "Sonic conserve sa vitesse en commençant une attaque glissée." } },
    { ELanguage::Spanish,  { "Conservar velocidad al derrapar", "Sonic conserva su velocidad al iniciar un ataque derrape." } },
    { ELanguage::Italian,  { "Mantieni velocità in scivolata", "Sonic mantiene la velocità quando inizia una Scivolata." } }
};

CONFIG_DEFINE_LOCALE(UnlimitedAntigravity)
{
    { ELanguage::English,  { "Unlimited Sliding Attack", "Stops the X-button Sliding Attack from ending automatically. With B controls, releasing B still ends the slide." } },
    { ELanguage::Japanese, { "スライディング時間の制限解除", "X操作のスライディングアタックが自動で終了しなくなります。B操作ではBを離すと終了します" } },
    { ELanguage::German,   { "Unbegrenzter Schlitterangriff", "Der Schlitterangriff mit X endet nicht automatisch. Bei B-Steuerung endet er beim Loslassen von B." } },
    { ELanguage::French,   { "Attaque glissée illimitée", "L'attaque glissée avec X ne se termine plus automatiquement. Avec la commande B, relâche B pour l'arrêter." } },
    { ELanguage::Spanish,  { "Ataque derrape ilimitado", "El ataque derrape con X deja de terminar automáticamente. Con el control B, soltar B sigue cancelándolo." } },
    { ELanguage::Italian,  { "Scivolata illimitata", "La Scivolata con X non termina più automaticamente. Con il comando B, rilascia B per fermarla." } }
};

CONFIG_DEFINE_LOCALE(AlwaysExitVehicles)
{
    { ELanguage::English,  { "Always Exit Vehicles", "Lets Shadow get out of vehicles at any time." } },
    { ELanguage::Japanese, { "乗り物からいつでも降りる", "シャドウがいつでも乗り物から降りられます" } },
    { ELanguage::German,   { "Fahrzeuge jederzeit verlassen", "Shadow kann jederzeit aus Fahrzeugen aussteigen." } },
    { ELanguage::French,   { "Quitter les véhicules à tout moment", "Shadow peut descendre des véhicules à tout moment." } },
    { ELanguage::Spanish,  { "Salir de vehículos en cualquier momento", "Shadow puede bajarse de los vehículos en cualquier momento." } },
    { ELanguage::Italian,  { "Scendi sempre dai veicoli", "Shadow può scendere dai veicoli in qualsiasi momento." } }
};

CONFIG_DEFINE_LOCALE(DisableChaosAttack)
{
    { ELanguage::English,  { "Disable Chaos Attack", "Shadow uses a standard Homing Attack in place of Chaos Attack." } },
    { ELanguage::Japanese, { "カオスアタックを無効化", "シャドウがカオスアタックの代わりに通常のホーミングアタックを使います" } },
    { ELanguage::German,   { "Chaos-Angriff deaktivieren", "Shadow nutzt einen normalen Zielangriff anstelle des Chaos-Angriffs." } },
    { ELanguage::French,   { "Désactiver l'attaque du chaos", "Shadow utilise une attaque téléguidée normale à la place de l'attaque du chaos." } },
    { ELanguage::Spanish,  { "Desactivar Ataque Caos", "Shadow usa un ataque teledirigido normal en lugar del Ataque Caos." } },
    { ELanguage::Italian,  { "Disattiva Attacco Chaos", "Shadow usa un normale attacco a ricerca al posto dell'Attacco Chaos." } }
};

CONFIG_DEFINE_LOCALE(IncreasedChaosBlastRange)
{
    { ELanguage::English,  { "Wider Chaos Blast", "Increases the range of Shadow's Chaos Blast." } },
    { ELanguage::Japanese, { "カオスブラストの範囲拡大", "シャドウのカオスブラストの効果範囲を広げます" } },
    { ELanguage::German,   { "Größere Chaos-Blast-Reichweite", "Erhöht die Reichweite von Shadows Chaos Blast." } },
    { ELanguage::French,   { "Chaos Blast élargi", "Augmente la portée du Chaos Blast de Shadow." } },
    { ELanguage::Spanish,  { "Mayor alcance de Chaos Blast", "Aumenta el alcance del Chaos Blast de Shadow." } },
    { ELanguage::Italian,  { "Raggio di Chaos Blast esteso", "Aumenta la portata del Chaos Blast di Shadow." } }
};

CONFIG_DEFINE_LOCALE(RestoreChaosBoostJump)
{
    { ELanguage::English,  { "Restore Chaos Boost Jump", "Restores Shadow's jump animation when activating Chaos Boost." } },
    { ELanguage::Japanese, { "カオスブーストのジャンプを復元", "カオスブースト発動時のシャドウのジャンプ演出を復元します" } },
    { ELanguage::German,   { "Chaos-Boost-Sprung wiederherstellen", "Stellt Shadows Sprunganimation beim Aktivieren des Chaos Boost wieder her." } },
    { ELanguage::French,   { "Rétablir le saut du Chaos Boost", "Rétablit l'animation de saut de Shadow à l'activation du Chaos Boost." } },
    { ELanguage::Spanish,  { "Restaurar salto de Chaos Boost", "Recupera la animación de salto de Shadow al activar Chaos Boost." } },
    { ELanguage::Italian,  { "Ripristina salto del Chaos Boost", "Ripristina l'animazione di salto di Shadow quando attiva il Chaos Boost." } }
};

CONFIG_DEFINE_LOCALE(RestoreChaosSpearFlips)
{
    { ELanguage::English,  { "Restore Chaos Spear Flips", "Restores Shadow's flips when using Chaos Spear." } },
    { ELanguage::Japanese, { "カオススピアの宙返りを復元", "カオススピア使用時のシャドウの宙返りを復元します" } },
    { ELanguage::German,   { "Chaos-Spear-Salti wiederherstellen", "Stellt Shadows Salti beim Einsatz von Chaos Spear wieder her." } },
    { ELanguage::French,   { "Rétablir les saltos du Chaos Spear", "Rétablit les saltos de Shadow lors de l'utilisation de Chaos Spear." } },
    { ELanguage::Spanish,  { "Restaurar volteretas de Chaos Spear", "Recupera las volteretas de Shadow al usar Chaos Spear." } },
    { ELanguage::Italian,  { "Ripristina capriole del Chaos Spear", "Ripristina le capriole di Shadow quando usa Chaos Spear." } }
};

CONFIG_DEFINE_LOCALE(AlwaysHoldPhysicsObjects)
{
    { ELanguage::English,  { "Hold Objects While Levitating", "Lets Silver hold physics objects while levitating." } },
    { ELanguage::Japanese, { "浮遊中に物を持つ", "シルバーが浮遊中に物を保持できます" } },
    { ELanguage::German,   { "Objekte beim Schweben halten", "Silver kann beim Schweben Physikobjekte festhalten." } },
    { ELanguage::French,   { "Tenir des objets en lévitation", "Silver peut tenir des objets physiques pendant qu'il lévite." } },
    { ELanguage::Spanish,  { "Sujetar objetos al levitar", "Silver puede sujetar objetos físicos mientras levita." } },
    { ELanguage::Italian,  { "Tieni gli oggetti in levitazione", "Silver può tenere oggetti fisici mentre levita." } }
};

CONFIG_DEFINE_LOCALE(ControllablePsychoShock)
{
    { ELanguage::English,  { "Controllable Psycho Shock", "Lets Silver move during Psycho Shock." } },
    { ELanguage::Japanese, { "サイコショックの操作", "サイコショック中にシルバーを移動させられます" } },
    { ELanguage::German,   { "Steuerbarer Psycho Shock", "Silver kann sich während Psycho Shock bewegen." } },
    { ELanguage::French,   { "Psycho Shock contrôlable", "Silver peut se déplacer pendant Psycho Shock." } },
    { ELanguage::Spanish,  { "Psycho Shock controlable", "Silver puede moverse durante Psycho Shock." } },
    { ELanguage::Italian,  { "Psycho Shock controllabile", "Silver può muoversi mentre usa Psycho Shock." } }
};

CONFIG_DEFINE_LOCALE(ControllableTeleportDash)
{
    { ELanguage::English,  { "Controllable Teleport Dash", "Lets Silver steer during Teleport Dash." } },
    { ELanguage::Japanese, { "テレポートダッシュの操作", "テレポートダッシュ中にシルバーの進行方向を変えられます" } },
    { ELanguage::German,   { "Steuerbarer Teleport Dash", "Silver kann während Teleport Dash die Richtung ändern." } },
    { ELanguage::French,   { "Teleport Dash contrôlable", "Silver peut changer de direction pendant Teleport Dash." } },
    { ELanguage::Spanish,  { "Teleport Dash controlable", "Silver puede cambiar de dirección durante Teleport Dash." } },
    { ELanguage::Italian,  { "Teleport Dash controllabile", "Silver può cambiare direzione durante il Teleport Dash." } }
};

CONFIG_DEFINE_LOCALE(RestoreSilverBossThrow)
{
    { ELanguage::English,  { "Restore Silver's Boss Throw", "Restores Silver's ability to throw bosses to damage them." } },
    { ELanguage::Japanese, { "シルバーのボス投げを復元", "シルバーがボスを投げてダメージを与える能力を復元します" } },
    { ELanguage::German,   { "Silvers Bosswurf wiederherstellen", "Silver kann Bosse wieder werfen, um ihnen Schaden zuzufügen." } },
    { ELanguage::French,   { "Rétablir la projection des boss", "Silver peut à nouveau projeter les boss pour leur infliger des dégâts." } },
    { ELanguage::Spanish,  { "Restaurar el lanzamiento de jefes", "Silver vuelve a poder lanzar a los jefes para hacerles daño." } },
    { ELanguage::Italian,  { "Ripristina il lancio dei boss", "Silver può di nuovo lanciare i boss per danneggiarli." } }
};

CONFIG_DEFINE_LOCALE(ControllableOmegaLauncher)
{
    { ELanguage::English,  { "Move During Omega Launcher", "Lets Omega move while shooting in midair." } },
    { ELanguage::Japanese, { "オメガランチャーの操作", "オメガが空中で射撃しながら移動できます" } },
    { ELanguage::German,   { "Steuerung beim Omega Launcher", "Omega kann sich beim Schießen in der Luft bewegen." } },
    { ELanguage::French,   { "Contrôle pendant Omega Launcher", "Omega peut se déplacer en tirant en plein saut." } },
    { ELanguage::Spanish,  { "Control durante Omega Launcher", "Omega puede moverse mientras dispara en el aire." } },
    { ELanguage::Italian,  { "Movimento durante Omega Launcher", "Omega può muoversi mentre spara in aria." } }
};

CONFIG_DEFINE_LOCALE(ControllableOmegaLockOnShot)
{
    { ELanguage::English,  { "Move During Omega Lock-On Shot", "Lets Omega move while firing homing lasers." } },
    { ELanguage::Japanese, { "オメガのロックオン射撃の操作", "オメガが追尾レーザーを発射しながら移動できます" } },
    { ELanguage::German,   { "Steuerung beim Omega-Ziellaser", "Omega kann sich beim Feuern von Zielsuchlasern bewegen." } },
    { ELanguage::French,   { "Contrôle du tir verrouillé d'Omega", "Omega peut se déplacer en tirant ses lasers à tête chercheuse." } },
    { ELanguage::Spanish,  { "Control del disparo fijado de Omega", "Omega puede moverse al disparar láseres teledirigidos." } },
    { ELanguage::Italian,  { "Movimento durante il tiro mirato", "Omega può muoversi mentre spara laser a ricerca." } }
};

CONFIG_DEFINE_LOCALE(FixOmegaShotCollision)
{
    { ELanguage::English,  { "Fix Omega Shot Collision", "Stops ally Omega's shots from intercepting Homing Attacks. Re-enter the stage if needed." } },
    { ELanguage::Japanese, { "オメガの弾の当たり判定を修正", "味方のオメガの弾にホーミングアタックが当たるのを防ぎます。必要に応じてステージに入り直してください" } },
    { ELanguage::German,   { "Kollision mit Omega-Schüssen", "Verhindert, dass Schüsse des verbündeten Omega Zielangriffe abfangen. Betritt den Level bei Bedarf erneut." } },
    { ELanguage::French,   { "Collision des tirs d'Omega", "Empêche les tirs d'Omega allié d'intercepter les attaques téléguidées. Relance le niveau si nécessaire." } },
    { ELanguage::Spanish,  { "Colisión de disparos de Omega", "Evita que los disparos de Omega aliado intercepten ataques teledirigidos. Vuelve a entrar en el nivel si hace falta." } },
    { ELanguage::Italian,  { "Collisione dei colpi di Omega", "Evita che i colpi di Omega alleato intercettino gli attacchi a ricerca. Rientra nel livello se necessario." } }
};

CONFIG_DEFINE_LOCALE(HorizontalCamera)
{
    { ELanguage::English,  { "Horizontal Camera", "Change how the camera moves left and right." } },
    { ELanguage::Japanese, { "カメラの左右", "カメラ左右の動く方向を選択できます" } },
    { ELanguage::German,   { "Horizontale Kamera", "Ändere wie sich die Kamera nach links und rechts bewegt." } },
    { ELanguage::French,   { "Caméra horizontale", "Modifie la rotation horizontale de la caméra." } },
    { ELanguage::Spanish,  { "Cámara horizontal", "Cambia cómo se mueve la camara hacia la izquierda y la derecha." } },
    { ELanguage::Italian,  { "Telecamera orizzontale", "Modifica come la telecamera si muove da sinistra a destra." } }
};

CONFIG_DEFINE_LOCALE(VerticalCamera)
{
    { ELanguage::English,  { "Vertical Camera", "Change how the camera moves up and down." } },
    { ELanguage::Japanese, { "カメラの上下", "カメラ上下の動く方向を選択できます" } },
    { ELanguage::German,   { "Vertikale Kamera", "Ändere wie sich die Kamera nach oben und unten bewegt." } },
    { ELanguage::French,   { "Caméra verticale", "Modifie la rotation verticale de la caméra." } },
    { ELanguage::Spanish,  { "Cámara vertical", "Cambia cómo se mueve la camara hacia arriba y abajo." } },
    { ELanguage::Italian,  { "Telecamera verticale", "Modifica come la telecamera si muove su e giù." } }
};

CONFIG_DEFINE_ENUM_LOCALE(ECameraRotationMode)
{
    {
        ELanguage::English,
        {
            { ECameraRotationMode::Normal,  { "Normal", "" } },
            { ECameraRotationMode::Reverse, { "Reverse", "" } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { ECameraRotationMode::Normal,  { "ノーマル", "" } },
            { ECameraRotationMode::Reverse, { "リバース", "" } }
        }
    },
    {
        ELanguage::German,
        {
            { ECameraRotationMode::Normal,  { "Normal", "" } },
            { ECameraRotationMode::Reverse, { "Invertiert", "" } }
        }
    },
    {
        ELanguage::French,
        {
            { ECameraRotationMode::Normal,  { "Normale", "" } },
            { ECameraRotationMode::Reverse, { "Inversée", "" } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { ECameraRotationMode::Normal,  { "Normal", "" } },
            { ECameraRotationMode::Reverse, { "Invertido", "" } }
        }
    },
    {
        ELanguage::Italian,
        {
            { ECameraRotationMode::Normal,  { "Normale", "" } },
            { ECameraRotationMode::Reverse, { "Invertita", "" } }
        }
    }
};

CONFIG_DEFINE_LOCALE(MouseSensitivity)
{
    { ELanguage::English,  { "Mouse Sensitivity", "Adjust the sensitivity of the mouse when moving the camera." } },
    { ELanguage::Japanese, { "マウス感度", "マウスでカメラの動かす速度を調整できます" } },
    { ELanguage::German,   { "Mausempfindlichkeit", "Passe die Empfindlichkeit der Maus beim Bewegen der Kamera an." } },
    { ELanguage::French,   { "Sensibilité de la souris", "Régle la sensibilité de la souris pendant le déplacement de la caméra." } },
    { ELanguage::Spanish,  { "Sensibilidad del ratón", "Ajusta la sensibilidad del ratón al mover la cámara." } },
    { ELanguage::Italian,  { "Sensibilità del mouse", "Regola la sensibilità del mouse per il movimento della telecamera." } }
};

CONFIG_DEFINE_LOCALE(MouseAutoCenter)
{
    { ELanguage::English,  { "Mouse Auto-Center", "Center the camera automatically when the mouse is not in use." } },
    { ELanguage::Japanese, { "マウスの自動中央揃え", "マウスが使用されていない場合、カメラを自動的に中央に配置します" } },
    { ELanguage::German,   { "Automatische Mauszentrierung", "Die Kamera automatisch zentrieren, wenn die Maus nicht verwendet wird." } },
    { ELanguage::French,   { "Centrage automatique de la souris", "Centre la caméra automatiquement quand la souris n'est pas utilisée." } },
    { ELanguage::Spanish,  { "Autocentrado del ratón", "Centra la cámara automáticamente cuando el ratón no esté en uso." } },
    { ELanguage::Italian,  { "Centraggio automatico del Mouse", "Centra automaticamente la telecamera quando il mouse non è in uso." } }
};

CONFIG_DEFINE_LOCALE(AllowBackgroundInput)
{
    { ELanguage::English,  { "Allow Background Input", "Allow controller input whilst the game window is unfocused." } },
    { ELanguage::Japanese, { "バックグラウンド入力", "フォーカスされていないゲームに入力できるか選択できます" } },
    { ELanguage::German,   { "Erlaube Hintergrundeingaben", "Erlaube Eingaben deines Controllers auch wenn das Spielfenster nicht fokussiert ist." } },
    { ELanguage::French,   { "Manette en arrière plan", "Permet d'utiliser la manette dans le jeu lorsque qu'il n'est pas au premier plan." } },
    { ELanguage::Spanish,  { "Control en segundo plano", "Permite controlar el juego con un mando mientras la ventana esté en segundo plano." } },
    { ELanguage::Italian,  { "Input con la finestra inattiva", "Attiva/disattiva i tasti del controller mentre la finestra è inattiva." } }
};

CONFIG_DEFINE_LOCALE(ControllerIcons)
{
    { ELanguage::English,  { "Controller Icons", "Change the icons to match your controller." } },
    { ELanguage::Japanese, { "コントローラーアイコン", "ゲーム内のコントローラーアイコンを変更できます" } },
    { ELanguage::German,   { "Controllersymbole", "Ändere die Controllersymbole, um sie auf dein Modell anzupassen." } },
    { ELanguage::French,   { "Icône des boutons", "Modifie les icônes pour les faire correspondre à votre manette." } },
    { ELanguage::Spanish,  { "Iconos del mando", "Cambia los iconos para que coincidan con tu mando." } },
    { ELanguage::Italian,  { "Icone dei tasti", "Modifica le icone per farle corrispondere con il tuo controller." } }
};

CONFIG_DEFINE_ENUM_LOCALE(EControllerIcons)
{
    {
        ELanguage::English,
        {
            { EControllerIcons::Auto,        { "Auto", "Auto: the game will determine which icons to use based on the current input device." } },
            { EControllerIcons::Xbox,        { "Xbox", "" } },
            { EControllerIcons::PlayStation, { "PlayStation", "" } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { EControllerIcons::Auto,        { "自動検出", "自動検出: コントローラーアイコンを使用している入力デバイスに応じて自動的に決定します" } },
            { EControllerIcons::Xbox,        { "Xbox", "" } },
            { EControllerIcons::PlayStation, { "PlayStation", "" } }
        }
    },
    {
        ELanguage::German,
        {
            { EControllerIcons::Auto,        { "Auto", "Auto: Das Spiel erkennt automatisch deinen Controller um die Symbole dementsprechend anzupassen." } },
            { EControllerIcons::Xbox,        { "Xbox", "" } },
            { EControllerIcons::PlayStation, { "PlayStation", "" } }
        }
    },
    {
        ELanguage::French,
        {
            { EControllerIcons::Auto,        { "Auto", "Auto : le jeu déterminera automatiquement quelles icônes utiliser en fonction du périphérique d'entrée." } },
            { EControllerIcons::Xbox,        { "Xbox", "" } },
            { EControllerIcons::PlayStation, { "PlayStation", "" } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { EControllerIcons::Auto,        { "Auto", "Auto: el juego determinará de forma automática qué iconos utilizar dependiendo del dispositivo de entrada actual." } },
            { EControllerIcons::Xbox,        { "Xbox", "" } },
            { EControllerIcons::PlayStation, { "PlayStation", "" } }
        }
    },
    {
        ELanguage::Italian,
        {
            { EControllerIcons::Auto,        { "Auto", "Auto: il gioco determinerà quali icone da utilizzare in base al dispositivo di input attuale." } },
            { EControllerIcons::Xbox,        { "Xbox", "" } },
            { EControllerIcons::PlayStation, { "PlayStation", "" } }
        }
    }
};

CONFIG_DEFINE_LOCALE(LightDash)
{
    { ELanguage::English,  { "Light Dash", "Change how Light Dash is activated for Sonic and Shadow." } },
    { ELanguage::Japanese, { "ライトダッシュ", "ソニックとシャドウのライトダッシュの発動方法を変更します" } },
    { ELanguage::German,   { "Lichtsprint", "Ändere wie der Lichtsprint von Sonic und Shadow aktiviert wird." } },
    { ELanguage::French,   { "Course éclair", "Modifier la façon dont Course éclair est activée pour Sonic et Shadow." } },
    { ELanguage::Spanish,  { "Acelerón ligero", "Cambia cómo se activa el acelerón ligero para Sonic y Shadow." } },
    { ELanguage::Italian,  { "Super spinta", "Cambia il modo in cui viene attivata la Super spinta per Sonic e Shadow." } }
};

CONFIG_DEFINE_ENUM_LOCALE(ELightDash)
{
    {
        ELanguage::English,
        {
            { ELightDash::X, { "Press ${picture(button_x)}", "" } },
            { ELightDash::Y, { "Press ${picture(button_y)}", "" } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { ELightDash::X, { "${picture(button_x)}を押す", "" } },
            { ELightDash::Y, { "${picture(button_y)}を押す", "" } }
        }
    },
    {
        ELanguage::German,
        {
            { ELightDash::X, { "Drücke ${picture(button_x)}", "" } },
            { ELightDash::Y, { "Drücke ${picture(button_y)}", "" } }
        }
    },
    {
        ELanguage::French,
        {
            { ELightDash::X, { "Appuyer sur ${picture(button_x)}", "" } },
            { ELightDash::Y, { "Appuyer sur ${picture(button_y)}", "" } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { ELightDash::X, { "Pulsar ${picture(button_x)}", "" } },
            { ELightDash::Y, { "Pulsar ${picture(button_y)}", "" } }
        }
    },
    {
        ELanguage::Italian,
        {
            { ELightDash::X, { "Premi ${picture(button_x)}", "" } },
            { ELightDash::Y, { "Premi ${picture(button_y)}", "" } }
        }
    }
};

CONFIG_DEFINE_LOCALE(SlidingAttack)
{
    { ELanguage::English,  { "Sliding Attack", "Change how the Sliding Attack is activated for Sonic." } },
    { ELanguage::Japanese, { "スライディングアタック", "ソニックのスライディングアタックの発動方法を変更します" } },
    { ELanguage::German,   { "Schlitterangriff", "Ändere wie der Schlitterangriff von Sonic aktiviert wird." } },
    { ELanguage::French,   { "Attaque-dérapage", "Modifier la façon dont l'Attaque-dérapage est activée pour Sonic." } },
    { ELanguage::Spanish,  { "Ataque derrape", "Cambia cómo se activa el ataque derrape para Sonic." } },
    { ELanguage::Italian,  { "Scivolata", "Cambia il modo in cui viene attivata la Scivolata per Sonic." } }
};

CONFIG_DEFINE_ENUM_LOCALE(ESlidingAttack)
{
    {
        ELanguage::English,
        {
            { ESlidingAttack::B, { "Hold ${picture(button_b)}", "" } },
            { ESlidingAttack::X, { "Release ${picture(button_x)}", "" } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { ESlidingAttack::B, { "${picture(button_b)}を押し続ける", "" } },
            { ESlidingAttack::X, { "${picture(button_x)}を離す", "" } }
        }
    },
    {
        ELanguage::German,
        {
            { ESlidingAttack::B, { "Halte ${picture(button_b)} gedrückt", "" } },
            { ESlidingAttack::X, { "Lasse ${picture(button_x)} los", "" } }
        }
    },
    {
        ELanguage::French,
        {
            { ESlidingAttack::B, { "Maintenir ${picture(button_b)}", "" } },
            { ESlidingAttack::X, { "Relâcher ${picture(button_x)}", "" } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { ESlidingAttack::B, { "Mantener ${picture(button_b)}", "" } },
            { ESlidingAttack::X, { "Soltar ${picture(button_x)}", "" } }
        }
    },
    {
        ELanguage::Italian,
        {
            { ESlidingAttack::B, { "Tieni premuto ${picture(button_b)}", "" } },
            { ESlidingAttack::X, { "Rilascia ${picture(button_x)}", "" } }
        }
    }
};

CONFIG_DEFINE_LOCALE(MasterVolume)
{
    { ELanguage::English,  { "Master Volume", "Adjust the overall volume." } },
    { ELanguage::Japanese, { "マスターボリューム", "全体ボリュームの大きさを調整できます" } },
    { ELanguage::German,   { "Gesamtlautstärke", "Passe die Gesamtlautstärke an." } },
    { ELanguage::French,   { "Volume général", "Réglage du volume général." } },
    { ELanguage::Spanish,  { "Volumen maestro", "Ajusta el volumen general." } },
    { ELanguage::Italian,  { "Volume principale", "Regola il volume principale" } }
};

CONFIG_DEFINE_LOCALE(MusicVolume)
{
    { ELanguage::English,  { "Music Volume", "Adjust the volume for the music." } },
    { ELanguage::Japanese, { "BGMボリューム", "BGMボリュームの大きさを調整できます" } },
    { ELanguage::German,   { "Musiklautstärke", "Passe die Lautstärke der Musik an." } },
    { ELanguage::French,   { "Volume de la musique", "Réglage du volume de la musique." } },
    { ELanguage::Spanish,  { "Volumen de la música", "Ajusta el volumen de la música." } },
    { ELanguage::Italian,  { "Volume musica di sottofondo", "Regola il volume della musica di sottofondo." } }
};

CONFIG_DEFINE_LOCALE(EffectsVolume)
{
    { ELanguage::English,  { "Effects Volume", "Adjust the volume for sound effects." } },
    { ELanguage::Japanese, { "SEボリューム", "SEボリュームの大きさを調整できます" } },
    { ELanguage::German,   { "Soundeffektlautstärke", "Passe die Lautstärke der Soundeffekte an." } },
    { ELanguage::French,   { "Volume des effets sonores", "Réglage du volume des effets sonores." } },
    { ELanguage::Spanish,  { "Volumen de efectos", "Ajusta el volumen de los efectos de sonido." } },
    { ELanguage::Italian,  { "Volume effetti sonori", "Regola il volume degli effetti sonori." } }
};

CONFIG_DEFINE_LOCALE(ChannelConfiguration)
{
    { ELanguage::English,  { "Channel Configuration", "Change the output mode for your audio device." } },
    { ELanguage::Japanese, { "チャンネル設定", "オーディオデバイスの出力モードを変更できます" } },
    { ELanguage::German,   { "Kanalkonfiguration", "Ändere den Ausgabemodus für dein Audioausgabegerät." } },
    { ELanguage::French,   { "Configuration sortie audio", "Modifie le mode de sortie pour votre périphérique audio." } },
    { ELanguage::Spanish,  { "Configuración de canales", "Cambia el modo de salida para tu dispositivo de audio." } },
    { ELanguage::Italian,  { "Configurazione canali audio", "Modifica la modalità di output per il tuo dispositivo audio." } }
};

CONFIG_DEFINE_ENUM_LOCALE(EChannelConfiguration)
{
    {
        ELanguage::English,
        {
            { EChannelConfiguration::Stereo,   { "Stereo", "" } },
            { EChannelConfiguration::Surround, { "Surround", "" } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { EChannelConfiguration::Stereo,   { "ステレオ", "" } },
            { EChannelConfiguration::Surround, { "サラウンド", "" } }
        }
    },
    {
        ELanguage::German,
        {
            { EChannelConfiguration::Stereo,   { "Stereo", "" } },
            { EChannelConfiguration::Surround, { "Surround", "" } }
        }
    },
    {
        ELanguage::French,
        {
            { EChannelConfiguration::Stereo,   { "Stéréo", "" } },
            { EChannelConfiguration::Surround, { "Surround", "" } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { EChannelConfiguration::Stereo,   { "Estéreo", "" } },
            { EChannelConfiguration::Surround, { "Envolvente", "" } }
        }
    },
    {
        ELanguage::Italian,
        {
            { EChannelConfiguration::Stereo,   { "Stereo", "" } },
            { EChannelConfiguration::Surround, { "Surround", "" } }
        }
    }
};

CONFIG_DEFINE_LOCALE(MuteOnFocusLost)
{
    { ELanguage::English,  { "Mute on Focus Lost", "Mute the game's audio when the window is not in focus." } },
    { ELanguage::Japanese, { "フォーカスロストミュート", "ウィンドウがフォーカスされていないときにゲームのオーディオをミュートします" } },
    { ELanguage::German,   { "Stummstellen wenn nicht fokussiert", "Stellt das Audio des Spiels stumm solange das Fenster nicht im Fokus ist." } },
    { ELanguage::French,   { "Muet si fenêtre inactive", "Coupe le son du jeu lorsque la fenêtre n'est pas active." } },
    { ELanguage::Spanish,  { "Silenciar al perder el foco", "Silencia el audio del juego cuando la ventana no esté en primer plano." } },
    { ELanguage::Italian,  { "Audio muto con finestra inattiva", "Disattiva l'audio del gioco quando la finestra non è attiva." } }
};

CONFIG_DEFINE_LOCALE(MusicAttenuation)
{
    { ELanguage::English,  { "Music Attenuation", "Fade out the game's music when external media is playing." } },
    { ELanguage::Japanese, { "BGM減衰", "外部メディアを再生するとゲームの音楽をフェードアウトします" } },
    { ELanguage::German,   { "Musikdämpfung", "Stelle die Musik des Spiels stumm während externe Medien abgespielt werden." } },
    { ELanguage::French,   { "Atténuation audio", "Abaisse le volume des musiques du jeu lorsqu'un média externe est en cours de lecture." } },
    { ELanguage::Spanish,  { "Atenuación de música", "Atenúa la música del juego cuando un reproductor multimedia se encuentra activo." } },
    { ELanguage::Italian,  { "Attenuazione musica", "Abbassa il volume della musica di sottofondo quando un'altra applicazione riproduce dei suoni." } }
};

CONFIG_DEFINE_LOCALE(WindowSize)
{
    { ELanguage::English,  { "Window Size", "Adjust the size of the game window in windowed mode." } },
    { ELanguage::Japanese, { "ウィンドウサイズ", "ウィンドウモードでのゲームのウィンドウサイズを調整できます" } },
    { ELanguage::German,   { "Fenstergröße", "Ändere die Größe des Spielfensters im Fenstermodus." } },
    { ELanguage::French,   { "Taille de la fenêtre", "Modifie la taille de la fenêtre de jeu en mode fenêtré." } },
    { ELanguage::Spanish,  { "Tamaño de ventana", "Ajusta el tamaño de la ventana de juego." } },
    { ELanguage::Italian,  { "Dimensioni finestra", "Regola le dimensioni della finestra del gioco in modalità finestra." } }
};

CONFIG_DEFINE_LOCALE(Monitor)
{
    { ELanguage::English,  { "Monitor", "Change which monitor to display the game on." } },
    { ELanguage::Japanese, { "モニター選択", "ゲームを表示するモニターを変更できます" } },
    { ELanguage::German,   { "Monitor", "Ändere auf welchem Monitor das Spiel angezeigt wird." } },
    { ELanguage::French,   { "Moniteur", "Change le moniteur sur lequel le jeu sera affiché." } },
    { ELanguage::Spanish,  { "Pantalla", "Cambia la pantalla en la cuál se muestra el juego." } },
    { ELanguage::Italian,  { "Schermo", "Cambia lo schermo su cui visualizzare il gioco." } }
};

CONFIG_DEFINE_LOCALE(AspectRatio)
{
    { ELanguage::English,  { "Aspect Ratio", "Change the aspect ratio." } },
    { ELanguage::Japanese, { "アスペクト比", "アスペクト比を変更できます" } },
    { ELanguage::German,   { "Seitenverhältnis", "Verändere das Seitenverhältnis." } },
    { ELanguage::French,   { "Format d'image", "Modifie le format d'image." } },
    { ELanguage::Spanish,  { "Relación de aspecto", "Cambia la relación de aspecto." } },
    { ELanguage::Italian,  { "Rapporto d'aspetto", "Modifica il rapporto d'aspetto." } }
};

CONFIG_DEFINE_ENUM_LOCALE(EAspectRatio)
{
    {
        ELanguage::English,
        {
            { EAspectRatio::Auto,     { "Auto", "Auto: the aspect ratio will dynamically adjust to the window size." } },
            { EAspectRatio::Original, { "Original", "Original: locks the game to a widescreen aspect ratio." } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { EAspectRatio::Auto,     { "自動", "自動: アスペクト比はウィンドウサイズに合わせて調整されます" } },
            { EAspectRatio::Original, { "オリジナル", "オリジナル: ワイドスクリーンのアスペクト比に固定されます" } }
        }
    },
    {
        ELanguage::German,
        {
            { EAspectRatio::Auto,     { "Auto", "Auto: Das Seitenverhältnis passt sich automatisch der Fenstergröße an." } },
            { EAspectRatio::Original, { "Original", "Original: Stellt das Spiel in einem Breitbildschirm-Format dar." } }
        }
    },
    {
        ELanguage::French,
        {
            { EAspectRatio::Auto,     { "Auto", "Auto : le format d'image s'adapte automatiquement à la taille de la fenêtre." } },
            { EAspectRatio::Original, { "Original", "Original : force le jeu sur un format d'image large." } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { EAspectRatio::Auto,     { "Auto", "Auto: la relación de aspecto se ajusta de forma dinámica al tamaño de la ventana." } },
            { EAspectRatio::Original, { "Original", "Original: muestra el juego en relación de aspecto de pantalla ancha." } }
        }
    },
    {
        ELanguage::Italian,
        {
            { EAspectRatio::Auto,     { "Auto", "Auto: il rapporto d'aspetto verra cambiato automaticamente in base alle dimensioni della finestra." } },
            { EAspectRatio::Original, { "Originale", "Originale: blocca il gioco a un rapporto d'aspetto widescreen." } }
        }
    }
};

CONFIG_DEFINE_LOCALE(ResolutionScale)
{
    { ELanguage::English,  { "Resolution Scale", "Adjust the internal resolution of the game." } },
    { ELanguage::Japanese, { "解像度スケール", "ゲームの内部解像度を調整できます" } },
    { ELanguage::German,   { "Rendering-Auflösung", "Passe die Auflösung der internen Darstellung an." } },
    { ELanguage::French,   { "Échelle de rendu", "Modifie la résolution interne du jeu." } },
    { ELanguage::Spanish,  { "Escala de resolución", "Ajusta la resolución interna del juego." } },
    { ELanguage::Italian,  { "Scala risoluzione", "Regola la risoluzione interna del gioco." } }
};

CONFIG_DEFINE_LOCALE(Fullscreen)
{
    { ELanguage::English,  { "Fullscreen", "Toggle between borderless fullscreen or windowed mode." } },
    { ELanguage::Japanese, { "フルスクリーン", "ボーダーレスフルスクリーンかウィンドウモードを選択できます" } },
    { ELanguage::German,   { "Vollbild", "Wechsle zwischen dem randlosen Vollbildmodus und dem Fenstermodus." } },
    { ELanguage::French,   { "Plein écran", "Alterne entre le mode plein écran sans bordures et le mode fenêtré." } },
    { ELanguage::Spanish,  { "Pantalla completa", "Cambia entre modo de pantalla completa o ventana." } },
    { ELanguage::Italian,  { "Schermo pieno", "Attiva/disattiva tra modalità finestra senza cornice e modalità finestra." } }
};

CONFIG_DEFINE_LOCALE(VSync)
{
    { ELanguage::English,  { "V-Sync", "Synchronize the game to the refresh rate of the display to prevent screen tearing." } },
    { ELanguage::Japanese, { "垂直同期", "垂直同期の設定を変更できます" } },
    { ELanguage::German,   { "V-Sync", "Synchronisiere das Spiel mit der Bildwiederholrate deines Bildschirms um Bildverzerrungen zu vermeiden." } },
    { ELanguage::French,   { "V-Sync", "Synchronise le jeu avec la fréquence de rafraîchissement de l'écran pour éviter le screen tearing." } },
    { ELanguage::Spanish,  { "V-Sync", "Sincroniza el juego a la tasa de refresco de la pantalla para prevenir el rasgado de la imagen." } },
    { ELanguage::Italian,  { "V-Sync", "Sincronizza il gioco con la frequenza d'aggiornamento del display per evitare lo screen tearing." } }
};

CONFIG_DEFINE_LOCALE(FPS)
{
    { ELanguage::English,  { "FPS", "Set the max frame rate the game can run at. WARNING: this may introduce glitches at frame rates other than 60 FPS." } },
    { ELanguage::Japanese, { "フレームレート上限", "ゲームが実行できる最大フレームレートを設定します　警告:60FPS以外のフレームレートでは不具合が発生する可能性があります" } },
    { ELanguage::German,   { "FPS", "Bestimmt die maximale Bildwiederholrate. WARNUNG: es können Fehler bei einer Bildwiederholrate über 60 FPS auftreten." } },
    { ELanguage::French,   { "IPS", "Limiter le nombre d'images par seconde. ATTENTION : Cela peut provoquer des bugs à des fréquences autres que 60 IPS." } },
    { ELanguage::Spanish,  { "FPS", "Establece la tasa máxima de fotogramas a la que puede ejecutarse el juego. ADVERTENCIA: esto puede provocar fallos en velocidades distintas a 60 FPS." } },
    { ELanguage::Italian,  { "FPS", "Imposta il framerate massimo del gioco. ATTENZIONE: questa opzione può causare problemi con dei framerate rate superiori a 60 FPS." } }
};

CONFIG_DEFINE_LOCALE(Brightness)
{
    { ELanguage::English,  { "Brightness", "Adjust the brightness level." } },
    { ELanguage::Japanese, { "明るさの設定", "明るさレベルを調整します" } },
    { ELanguage::German,   { "Helligkeit", "Stelle die Helligkeit ein." } },
    { ELanguage::French,   { "Luminosité", "Modifier le niveau de luminosité." } },
    { ELanguage::Spanish,  { "Brillo", "Ajusta el nivel de brillo." } },
    { ELanguage::Italian,  { "Luminosità", "Regola il livello di luminosità." } }
};

CONFIG_DEFINE_LOCALE(AntiAliasing)
{
    { ELanguage::English,  { "Anti-Aliasing", "Adjust the amount of smoothing applied to jagged edges." } },
    { ELanguage::Japanese, { "アンチエイリアス", "アンチエイリアスの種類を選択できます" } },
    { ELanguage::German,   { "Kantenglättung", "Passe die Menge an Kantenglättung an." } },
    { ELanguage::French,   { "Anticrénelage", "Ajuste le niveau d'anticrénelage appliqué aux bords des objets." } },
    { ELanguage::Spanish,  { "Anti-Aliasing", "Ajusta el nivel de suavizado aplicado a los dientes de sierra." } },
    { ELanguage::Italian,  { "Anti-Aliasing", "Regola la quantità di smussamento applicata ai bordi." } }
};

CONFIG_DEFINE_ENUM_LOCALE(EAntiAliasing)
{
    {
        ELanguage::English,
        {
            { EAntiAliasing::Off, { "Off", "" } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { EAntiAliasing::Off, { "オフ", "" } },
        }
    },
    {
        ELanguage::German,
        {
            { EAntiAliasing::Off, { "Aus", "" } }
        }
    },
    {
        ELanguage::French,
        {
            { EAntiAliasing::Off, { "Non", "" } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { EAntiAliasing::Off, { "No", "" } },
        }
    },
    {
        ELanguage::Italian,
        {
            { EAntiAliasing::Off, { "No", "" } }
        }
    }
};

CONFIG_DEFINE_LOCALE(TransparencyAntiAliasing)
{
    { ELanguage::English,  { "Transparency Anti-Aliasing", "Apply anti-aliasing to alpha transparent textures." } },
    { ELanguage::Japanese, { "透明度のアンチエイリアス", "透過テクスチャにアンチエイリアスを適用します" } },
    { ELanguage::German,   { "Transparenz-Kantenglättung", "Wende Kantenglättung auf Alpha-Transparenz-Texturen an." } },
    { ELanguage::French,   { "Anticrénelage de transparence", "Applique l'anticrénelage sur les textures transparentes." } },
    { ELanguage::Spanish,  { "Anti-Aliasing de transparencias", "Aplica antialiasing a las texturas transparentes." } },
    { ELanguage::Italian,  { "Anti-Aliasing su texture trasparenti", "Applica l'anti-aliasing alle texture trasparenti." } }
};

CONFIG_DEFINE_LOCALE(ShadowResolution)
{
    { ELanguage::English,  { "Shadow Resolution", "Set the resolution of real-time shadows." } },
    { ELanguage::Japanese, { "影の解像度", "影の解像度を設定できます" } },
    { ELanguage::German,   { "Schattenauflösung", "Stelle die Auflösung der Echtzeit-Schatten ein." } },
    { ELanguage::French,   { "Résolution des ombres", "Définit la résolution des ombres en temps réel." } },
    { ELanguage::Spanish,  { "Resolución de sombras", "Establece la resolución de las sombras en tiempo real." } },
    { ELanguage::Italian,  { "Risoluzione ombre", "Imposta la risoluzioni delle ombre in tempo reale." } }
};

CONFIG_DEFINE_ENUM_LOCALE(EShadowResolution) {};

CONFIG_DEFINE_LOCALE(ReflectionResolution)
{
    { ELanguage::English,  { "Reflection Resolution", "Set the resolution of real-time reflections." } },
    { ELanguage::Japanese, { "反射解像度", "リアルタイム反射の解像度を設定します" } },
    { ELanguage::German,   { "Reflektionsauflösung", "Bestimmt die Auflösung der Echtzeitreflektionen." } },
    { ELanguage::French,   { "Résolution des reflets", "Définir la résolution des reflets en temps réel." } },
    { ELanguage::Spanish,  { "Resolución de reflejos", "Establece la resolución de los reflejos en tiempo real." } },
    { ELanguage::Italian,  { "Risoluzione riflessi", "Imposta la risoluzione dei riflessi in tempo reale." } }
};

CONFIG_DEFINE_ENUM_LOCALE(EReflectionResolution)
{
    {
        ELanguage::English,
        {
            { EReflectionResolution::Off, { "Off", "" } },
            { EReflectionResolution::Eighth, { "12.5%", "" } },
            { EReflectionResolution::Quarter, { "25%", "" } },
            { EReflectionResolution::Half, { "50%", "" } },
            { EReflectionResolution::Full, { "100%", "" } }
        }
    },
    { ELanguage::Japanese, { { EReflectionResolution::Off, { "オフ", "" } } } },
    { ELanguage::German,   { { EReflectionResolution::Off, { "Aus", "" } } } },
    { ELanguage::French,   { { EReflectionResolution::Off, { "Non", "" } } } },
    { ELanguage::Spanish,  { { EReflectionResolution::Off, { "No", "" } } } },
    { ELanguage::Italian,  { { EReflectionResolution::Off, { "Off", "" } } } }
};

CONFIG_DEFINE_LOCALE(RadialBlur)
{
    { ELanguage::English,  { "Radial Blur", "Change the quality of the radial blur." } },
    { ELanguage::Japanese, { "放射状ぼかし", "放射状ぼかしの品質を変更します" } },
    { ELanguage::German,   { "Radiale Unschärfe", "Verändere die Qualität der radialen Unschärfe." } },
    { ELanguage::French,   { "Flou Radial", "Modifier la qualité du flou radial." } },
    { ELanguage::Spanish,  { "Desenfoque radial", "Cambia la calidad del desenfoque radial." } },
    { ELanguage::Italian,  { "Sfocatura radiale", "Modifica la qualità della sfocatura radiale." } }
};

CONFIG_DEFINE_ENUM_LOCALE(ERadialBlur)
{
    {
        ELanguage::English,
        {
            { ERadialBlur::Off,      { "Off", "" } },
            { ERadialBlur::Original, { "Original", "" } },
            { ERadialBlur::Enhanced, { "Enhanced", "Enhanced: uses more samples for smoother radial blur." } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { ERadialBlur::Off,      { "オフ", "" } },
            { ERadialBlur::Original, { "オリジナル", "" } },
            { ERadialBlur::Enhanced, { "エンハンスド", "エンハンスド：より多くのサンプルを使用してよりスムーズな放射状ぼかしを実現します" } }
        }
    },
    {
        ELanguage::German,
        {
            { ERadialBlur::Off,      { "Aus", "" } },
            { ERadialBlur::Original, { "Original", "" } },
            { ERadialBlur::Enhanced, { "Verbessert", "Verbessert: Benutzt mehr Samples um eine weichere radiale Unschärfe zu erzeugen." } }
        }
    },
    {
        ELanguage::French,
        {
            { ERadialBlur::Off,      { "Non", "" } },
            { ERadialBlur::Original, { "Original", "" } },
            { ERadialBlur::Enhanced, { "Amélioré", "Amélioré : utilise plus d'échantillons pour un flou radial plus lisse." } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { ERadialBlur::Off,      { "No", "" } },
            { ERadialBlur::Original, { "Original", "" } },
            { ERadialBlur::Enhanced, { "Mejorado", "Mejorado: utiliza más muestras para un desenfoque radial más suave." } }
        }
    },
    {
        ELanguage::Italian,
        {
            { ERadialBlur::Off,      { "No", "" } },
            { ERadialBlur::Original, { "Originale", "" } },
            { ERadialBlur::Enhanced, { "Migliorato", "Migliorato: utilizza più campioni per una sfocatura radiale più uniforme." } }
        }
    }
};

CONFIG_DEFINE_LOCALE(CutsceneAspectRatio)
{
    { ELanguage::English,  { "Cutscene Aspect Ratio", "Change the aspect ratio of the real-time cutscenes." } },
    { ELanguage::Japanese, { "アスペクト比のカットシーン", "リアルタイムカットシーンのアスペクト比を変更できます" } },
    { ELanguage::German,   { "Zwischensequenz-Seitenverhältnis", "Verändere das Seitenverhältnis der Echtzeit-Zwischensequenzen." } },
    { ELanguage::French,   { "Format des cinématiques", "Modifie le format d'image des cinématiques en temps réel." } },
    { ELanguage::Spanish,  { "Relación de aspecto de cinemáticas", "Cambia la relación de aspecto de las cinemáticas en tiempo real." } },
    { ELanguage::Italian,  { "Rapporto d'aspetto dei filmati", "Cambia il rapporto d'aspetto dei filmati in tempo reale." } }
};

CONFIG_DEFINE_ENUM_LOCALE(ECutsceneAspectRatio)
{
    {
        ELanguage::English,
        {
            { ECutsceneAspectRatio::Original, { "Original", "Original: locks cutscenes to their original 16:9 aspect ratio." } },
            { ECutsceneAspectRatio::Unlocked, { "Unlocked", "Unlocked: allows cutscenes to adjust their aspect ratio to the window size. WARNING: this will introduce visual oddities past the original 16:9 aspect ratio." } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { ECutsceneAspectRatio::Original, { "オリジナル", "オリジナル: カットシーンを元の16:9のアスペクト比に固定します" } },
            { ECutsceneAspectRatio::Unlocked, { "解除", "解除: カットシーンのアスペクト比をウィンドウサイズに合わせて調整します 警告: 元の16:9のアスペクト比を超えると視覚的な異常が発生します" } },
        }
    },
    {
        ELanguage::German,
        {
            { ECutsceneAspectRatio::Original, { "Original", "Original: Behält die Zwischensequenzen in ihrem originalen 16:9 Seitenverhältnis." } },
            { ECutsceneAspectRatio::Unlocked, { "Entsperrt", "Entsperrt: Erlaubt Zwischensequenzen ihr Seitenverhältnis der Fenstergröße anzupassen. WARNUNG: Diese Option kann zu visuellen Fehlern außerhalb der Grenzen des ursprünglichen Seitenverhältnisses führen." } }
        }
    },
    {
        ELanguage::French,
        {
            { ECutsceneAspectRatio::Original, { "Original", "Original : force les cinématiques dans leur format 16:9 d'origine." } },
            { ECutsceneAspectRatio::Unlocked, { "Libre", "Libre : permet aux cinématiques d'adapter leur format d'image à la taille de la fenêtre. ATTENTION : au delà du format 16:9 d'origine, des bugs visuels apparaitront." } },
        }
    },
    {
        ELanguage::Spanish,
        {
            { ECutsceneAspectRatio::Original, { "Original", "Original: muestra las cinemáticas en su relación de aspecto original de 16:9." } },
            { ECutsceneAspectRatio::Unlocked, { "Desbloqueado", "Desbloqueado: permite que las cinemáticas ajusten su relación de aspecto al tamaño de la ventana. ADVERTENCIA: esto introducirá rarezas visuales más allá de la relación de aspecto original de 16:9." } },
        }
    },
    {
        ELanguage::Italian,
        {
            { ECutsceneAspectRatio::Original, { "Originale", "Originale: blocca il rapporto d'aspetto dei filmati a 16:9." } },
            { ECutsceneAspectRatio::Unlocked, { "Sbloccato", "Sbloccato: il rapporto d'aspetto verrà regolato in base alle dimensioni della finestra. ATTENZIONE: questa opzione potrebbe causare dei problemi visivi se il rapporto d'aspetto è oltre 16:9." } }
        }
    }
};

CONFIG_DEFINE_LOCALE(UIAlignmentMode)
{
    { ELanguage::English,  { "UI Alignment Mode", "Change how the UI aligns with the display." } },
    { ELanguage::Japanese, { "UIアライメントモード", "UIとディスプレイの配置を変更できます" } },
    { ELanguage::German,   { "Benutzeroberflächenausrichtung", "Verändere wie die Benutzeroberfläche sich mit dem Bildschirm ausrichtet." } },
    { ELanguage::French,   { "Alignement de l'IU", "Modifie l'alignement de l'interface utilisateur sur l'écran." } },
    { ELanguage::Spanish,  { "Modo de alineamiento de UI", "Cambia la alineación de la interfaz de usuario con la pantalla." } },
    { ELanguage::Italian,  { "Modalità allineamento interfaccia", "Modifica come l'interfaccia si allinea con lo schermo." } }
};

CONFIG_DEFINE_ENUM_LOCALE(EUIAlignmentMode)
{
    {
        ELanguage::English,
        {
            { EUIAlignmentMode::Edge,    { "Edge", "Edge: the UI will align with the edges of the display." } },
            { EUIAlignmentMode::Centre,  { "Center", "Center: the UI will align with the center of the display." } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { EUIAlignmentMode::Edge,    { "エッジ", "エッジ: UIがディスプレイの端に揃います" } },
            { EUIAlignmentMode::Centre,  { "センター", "センター: UIがディスプレイの中央に揃います" } },
        }
    },
    {
        ELanguage::German,
        {
            { EUIAlignmentMode::Edge,    { "Rand", "Rand: Die Benutzeroberfläche richtet sich zum Rand des Bildschirms aus." } },
            { EUIAlignmentMode::Centre,  { "Mitte", "Mitte: Die Benutzeroberfläche richtet sich zur Mitte des Bildschirms aus." } }
        }
    },
    {
        ELanguage::French,
        {
            { EUIAlignmentMode::Edge,    { "Bord", "Bord : l'interface utilisateur sera alignée sur les bords de l'écran." } },
            { EUIAlignmentMode::Centre,  { "Centrée", "Centrée : l'interface utilisateur sera alignée sur le centre de l'écran." } },
        }
    },
    {
        ELanguage::Spanish,
        {
            { EUIAlignmentMode::Edge,    { "Borde", "Borde: la interfaz de usuario se alineará con los bordes de la pantalla." } },
            { EUIAlignmentMode::Centre,  { "Centro", "Centro: la interfaz de usuario se alineará con el centro de la pantalla." } },
        }
    },
    {
        ELanguage::Italian,
        {
            { EUIAlignmentMode::Edge,    { "Bordi", "Bordi: l'interfaccia si allineerà con i bordi dello schermo." } },
            { EUIAlignmentMode::Centre,  { "Centro", "Centro: l'interfaccia si allineerà con il centro dello schermo." } }
        }
    }
};
