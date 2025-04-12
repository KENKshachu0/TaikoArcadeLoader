#pragma once
#include <xxhash.h>

enum class GameVersion : XXH64_hash_t {
    UNKNOWN = 0,
    JPN00   = 0x4C07355966D815FB,
    JPN08   = 0x67C0F3042746D488,
    JPN39   = 0x49F643ADB6B18705,
    CHN00   = 0xA7EE39F2CC2C57C8,
};

enum StatusType {
    CardStatus = 1,
    QrStatus   = 2
};

enum SceneStatus {
    SceneNone = -1,
    SceneLoading = 0,
    SceneTestMode,
    SceneNotice,
    SceneCopyright,
    SceneBNLogo,
    SceneTitle,
    SceneAttractMovie,
    SceneMusicIntro,
    SceneCaution,
    SceneEntry = 50,
    SceneSelectAiEnso = 100,
    SceneStartAiEnso,
    SceneResultAiEnso,
    SceneResultAllAiEnso,
    SceneSelectKimetsu = 200,
    SceneStartKimetsu,
    SceneResultKimetsu,
    SceneSelectOnePiece = 300,
    SceneStartOnePiece,
    SceneResultOnePiece,
    SceneSelectAiSoshina = 400,
    SceneStartAiSoshina,
    SceneResultAiSoshina,
    SceneSelectAoharu = 500,
    SceneStartAoharu,
    SceneResultAoharu,
    SceneSelectEnso = 10000,
    SceneStartEnso,
    SceneResultEnso,
    SceneSelectDani = 20000,
    SceneStartDani,
    SceneResultDani,
};