#include "constants.h"
#include "helpers.h"
#include "patches.h"

extern GameVersion gameVersion;

namespace patches::StatusMonitor {

SceneStatus currentScene = SceneStatus::SceneNone;

#define DEFINE_SCENE(sceneName)                                        \
typedef char (*sceneName) (i64 a1, i64 a2, i64 a3);                    \
SafetyHookInline original##sceneName;                                  \
char implOf##sceneName (i64 a1, i64 a2, i64 a3) {                      \
    char result = original##sceneName.fastcall <char>(a1, a2, a3);     \
    if (currentScene != SceneStatus::##sceneName) {                    \
        LogMessage (LogLevel::INFO, "Switch to {}", #sceneName);       \
        Plugins::UpdateScene (currentScene, SceneStatus::##sceneName); \
        currentScene = SceneStatus::##sceneName;                       \
    }                                                                  \
    return result;                                                     \
}
// Attract
DEFINE_SCENE (SceneLoading)
DEFINE_SCENE (SceneTestMode)
DEFINE_SCENE (SceneNotice)
DEFINE_SCENE (SceneCopyright)
DEFINE_SCENE (SceneBNLogo)
DEFINE_SCENE (SceneTitle)
DEFINE_SCENE (SceneAttractMovie)
DEFINE_SCENE (SceneMusicIntro)
DEFINE_SCENE (SceneCaution)
// Entry
DEFINE_SCENE (SceneEntry)
// AiEnso
DEFINE_SCENE (SceneSelectAiEnso)
DEFINE_SCENE (SceneStartAiEnso)
DEFINE_SCENE (SceneResultAiEnso)
DEFINE_SCENE (SceneResultAllAiEnso)
// Kimetsu
DEFINE_SCENE (SceneSelectKimetsu)
DEFINE_SCENE (SceneStartKimetsu)
DEFINE_SCENE (SceneResultKimetsu)
// OnePiece
DEFINE_SCENE (SceneSelectOnePiece)
DEFINE_SCENE (SceneStartOnePiece)
DEFINE_SCENE (SceneResultOnePiece)
// AiSoshina
DEFINE_SCENE (SceneSelectAiSoshina)
DEFINE_SCENE (SceneStartAiSoshina)
DEFINE_SCENE (SceneResultAiSoshina)
// Aoharu
DEFINE_SCENE (SceneSelectAoharu)
DEFINE_SCENE (SceneStartAoharu)
DEFINE_SCENE (SceneResultAoharu)
// Enso
DEFINE_SCENE (SceneSelectEnso)
DEFINE_SCENE (SceneStartEnso)
DEFINE_SCENE (SceneResultEnso)
// Dani
DEFINE_SCENE (SceneSelectDani)
DEFINE_SCENE (SceneStartDani)
DEFINE_SCENE (SceneResultDani)

void Init() {
    switch (gameVersion) {
        case GameVersion::JPN39: {
            // Attract
            INSTALL_FAST_HOOK_DYNAMIC (SceneTestMode,        ASLR (0x140479520));
            INSTALL_FAST_HOOK_DYNAMIC (SceneCopyright,       ASLR (0x140446B90));
            INSTALL_FAST_HOOK_DYNAMIC (SceneBNLogo,          ASLR (0x140433960));
            INSTALL_FAST_HOOK_DYNAMIC (SceneTitle,           ASLR (0x14045A4F0));
            INSTALL_FAST_HOOK_DYNAMIC (SceneAttractMovie,    ASLR (0x1404320C0));
            INSTALL_FAST_HOOK_DYNAMIC (SceneMusicIntro,      ASLR (0x1404572B0));
            INSTALL_FAST_HOOK_DYNAMIC (SceneCaution,         ASLR (0x140433E60));
            // Entry
            INSTALL_FAST_HOOK_DYNAMIC (SceneEntry,           ASLR (0x14044EBD0));
            // AiEnso
            INSTALL_FAST_HOOK_DYNAMIC (SceneSelectAiEnso,    ASLR (0x140424E20));
            INSTALL_FAST_HOOK_DYNAMIC (SceneStartAiEnso,     ASLR (0x14040F7B0));
            INSTALL_FAST_HOOK_DYNAMIC (SceneResultAiEnso,    ASLR (0x14041F130));
            INSTALL_FAST_HOOK_DYNAMIC (SceneResultAllAiEnso, ASLR (0x14041C900));
            // Kimetsu
            INSTALL_FAST_HOOK_DYNAMIC (SceneSelectKimetsu,   ASLR (0x140452DB0));
            INSTALL_FAST_HOOK_DYNAMIC (SceneStartKimetsu,    ASLR (0x14041B760));
            INSTALL_FAST_HOOK_DYNAMIC (SceneResultKimetsu,   ASLR (0x140450020));
            // OnePiece
            INSTALL_FAST_HOOK_DYNAMIC (SceneSelectOnePiece,  ASLR (0x140436770));
            INSTALL_FAST_HOOK_DYNAMIC (SceneSelectOnePiece,  ASLR (0x14041BA50));
            INSTALL_FAST_HOOK_DYNAMIC (SceneResultOnePiece,  ASLR (0x140434940));
            // AiSoshina
            INSTALL_FAST_HOOK_DYNAMIC (SceneSelectAiSoshina, ASLR (0x14043B3D0));
            INSTALL_FAST_HOOK_DYNAMIC (SceneStartAiSoshina,  ASLR (0x140410610));
            INSTALL_FAST_HOOK_DYNAMIC (SceneResultAiSoshina, ASLR (0x140439510));
            // Aoharu
            INSTALL_FAST_HOOK_DYNAMIC (SceneSelectAoharu,    ASLR (0x14042EA90));
            INSTALL_FAST_HOOK_DYNAMIC (SceneStartAoharu,     ASLR (0x14040FE50));
            INSTALL_FAST_HOOK_DYNAMIC (SceneResultAoharu,    ASLR (0x14042DB00));
            // Enso
            INSTALL_FAST_HOOK_DYNAMIC (SceneSelectEnso,      ASLR (0x1404584C0));
            INSTALL_FAST_HOOK_DYNAMIC (SceneStartEnso,       ASLR (0x1404114E0));
            INSTALL_FAST_HOOK_DYNAMIC (SceneResultEnso,      ASLR (0x140416760));
            // Dani
            INSTALL_FAST_HOOK_DYNAMIC (SceneSelectDani,      ASLR (0x14044BBA0));
            INSTALL_FAST_HOOK_DYNAMIC (SceneStartDani,       ASLR (0x140410BA0));
            INSTALL_FAST_HOOK_DYNAMIC (SceneResultDani,      ASLR (0x1404478B0));
        }
    }
}
}