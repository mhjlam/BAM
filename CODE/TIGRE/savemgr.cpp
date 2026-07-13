//
// SAVEMGR.CPP
//
// June 2, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// The main save manager functions.
//
// Save file format (Linux port, version 2) — two files per save slot:
//
//   N.sav  (plain text header, used by save-slot UI):
//     line 1: "BAMJ <version>"   e.g. "BAMJ 2"
//     line 2: save name (as typed by the user)
//
//   N.dat   (plain JSON text, valid JSON, human-readable/editable)
//
// Atomic write: each file is written to a .tmp sibling then rename()d.
//
//----[]-------------------------------------------------------------

#include "api.hpp"
#include "apimem.hpp"
#include "apires.hpp"
#include "savemgr.hpp"
#include "file.hpp"
#include <string.h>
#include <dirent.h>

static void build_save_path(char* buf, size_t sz, unsigned saveNum)
{
    snprintf(buf, sz, "%s%u.sav", get_pref_dir(), saveNum);
}

static void build_json_path(char* buf, size_t sz, unsigned saveNum)
{
    snprintf(buf, sz, "%s%u.dat", get_pref_dir(), saveNum);
}

#define BAM_SAVE_MAGIC   "BAMJ"
#define BAM_SAVE_VERSION ((uint16)2)

static bool write_save_header(FILE *fp, const char *saveName)
{
    return fprintf(fp, "%s %u\n%s\n", BAM_SAVE_MAGIC, (unsigned)BAM_SAVE_VERSION, saveName) < 0;
}

static uint16 read_save_header(FILE *fp, char *saveName, uint16 maxName,
                               uint16 *outVersion)
{
    char line1[64];
    if (!fgets(line1, sizeof(line1), fp)) return SM_FILE_READ_ERROR;

    char magic[8] = {};
    unsigned ver = 0;
    if (sscanf(line1, "%7s %u", magic, &ver) != 2) return SM_VERSION_ERROR;
    if (strcmp(magic, BAM_SAVE_MAGIC) != 0) return SM_VERSION_ERROR;
    if ((uint16)ver != BAM_SAVE_VERSION) return SM_VERSION_ERROR;
    if (outVersion) *outVersion = (uint16)ver;

    if (saveName && maxName > 0) {
        if (!fgets(saveName, maxName, fp)) { saveName[0] = '\0'; return SM_NO_ERROR; }
        size_t n = strlen(saveName);
        if (n > 0 && saveName[n-1] == '\n') saveName[n-1] = '\0';
    }

    return SM_NO_ERROR;
}


SaveMgr saveMgr;

extern uint16      atSaveCnt;
extern atSavePtr   atSaveArray[MAX_AT_SAVES];

SaveMgr::SaveMgr(void)
{
    pSaveMgrFile = nullptr;
    getFirstCalled = false;
    pSaveDir = nullptr;
}


SaveMgr::~SaveMgr(void)
{
    if (pSaveMgrFile != nullptr) {
        fclose(pSaveMgrFile);
        pSaveMgrFile = nullptr;
    }
    if (pSaveDir != nullptr) {
        closedir(pSaveDir);
        pSaveDir = nullptr;
    }
}


uint16
SaveMgr::Save(uint16 saveNum, uint16 /*versionNum*/, uint16 /*versionSubNum*/,
    uint16 /*buildID*/, char *saveName)
{
    char fileName[FILENAME_MAX];
    char tmpName[FILENAME_MAX];
    build_save_path(fileName, sizeof(fileName), (unsigned)saveNum);

    uint16 result = SM_NO_ERROR;
    nlohmann::json root = nlohmann::json::object();

    // Phase 1: BEFORE_SAVE (pause audio/events — doesn't write to json)
    if (ExecuteAtSaveFunctions(BEFORE_SAVE, root))
        result = SM_FILE_WRITE_ERROR;

    // Phase 2: DURING_SAVE (serialize all subsystems into root json)
    if (result == SM_NO_ERROR && ExecuteAtSaveFunctions(DURING_SAVE, root))
        result = SM_FILE_WRITE_ERROR;

    // Phase 3: AFTER_SAVE (resume audio/events — always runs to restore state)
    ExecuteAtSaveFunctions(AFTER_SAVE, root);

    // Write JSON atomically to N.json
    if (result == SM_NO_ERROR) {
        char jsonName[FILENAME_MAX], jsonTmp[FILENAME_MAX];
        build_json_path(jsonName, sizeof(jsonName), (unsigned)saveNum);
        snprintf(jsonTmp, sizeof(jsonTmp), "%s.tmp", jsonName);

        FILE *fp = fopen(jsonTmp, "w");
        if (!fp) {
            result = SM_FILE_OPEN_ERROR;
        } else {
            std::string json_str = root.dump(2);
            if (fwrite(json_str.c_str(), 1, json_str.size(), fp) != json_str.size())
                result = SM_FILE_WRITE_ERROR;
            fclose(fp);
            if (result == SM_NO_ERROR) {
                if (rename(jsonTmp, jsonName) != 0)
                    result = SM_FILE_WRITE_ERROR;
            } else {
                remove(jsonTmp);
            }
        }
    }

    // Write header atomically to N.sav
    if (result == SM_NO_ERROR) {
        snprintf(tmpName, sizeof(tmpName), "%s.tmp", fileName);
        FILE *fp = fopen(tmpName, "w");
        if (!fp) {
            result = SM_FILE_OPEN_ERROR;
        } else {
            if (write_save_header(fp, saveName))
                result = SM_FILE_WRITE_ERROR;
            fclose(fp);
            if (result == SM_NO_ERROR) {
                if (rename(tmpName, fileName) != 0)
                    result = SM_FILE_WRITE_ERROR;
            } else {
                remove(tmpName);
            }
        }
    }

    return result;
}


uint16
SaveMgr::Restore(uint16 saveNum, uint16 /*versionNum*/, uint16 /*versionSubNum*/,
    uint16 /*buildID*/)
{
    char fileName[FILENAME_MAX];
    build_save_path(fileName, sizeof(fileName), (unsigned)saveNum);

    // Validate header from N.sav
    FILE *fp = fopen(fileName, "r");
    if (!fp) return SM_FILE_OPEN_ERROR;
    uint16 result = read_save_header(fp, nullptr, 0, nullptr);
    fclose(fp);
    if (result != SM_NO_ERROR) return result;

    // Read JSON from N.json
    char jsonName[FILENAME_MAX];
    build_json_path(jsonName, sizeof(jsonName), (unsigned)saveNum);

    FILE *jfp = fopen(jsonName, "r");
    if (!jfp) return SM_FILE_OPEN_ERROR;

    fseek(jfp, 0, SEEK_END);
    long jsonLen = ftell(jfp);
    fseek(jfp, 0, SEEK_SET);

    std::string json_str((size_t)jsonLen, '\0');
    bool readOk = (long)fread(&json_str[0], 1, (size_t)jsonLen, jfp) == jsonLen;
    fclose(jfp);

    if (!readOk) return SM_FILE_READ_ERROR;

    try {
        nlohmann::json root = nlohmann::json::parse(json_str);

        if (ExecuteAtSaveFunctions(BEFORE_RESTORE, root))
            result = SM_FILE_READ_ERROR;
        else if (ExecuteAtSaveFunctions(DURING_RESTORE, root))
            result = SM_FILE_READ_ERROR;
        else if (ExecuteAtSaveFunctions(AFTER_RESTORE, root))
            result = SM_FILE_READ_ERROR;
    } catch (const nlohmann::json::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "BAM save: JSON parse error: %s", e.what());
        result = SM_FILE_READ_ERROR;
    }

    return result;
}


bool
SaveMgr::ExecuteAtSaveFunctions(uint16 saveState, nlohmann::json& root)
{
    // Last registered is called first (LIFO).
    for (int i = atSaveCnt - 1; i >= 0; i--) {
        if (atSaveArray[i](saveState, root))
            return true;
    }
    return false;
}


//--------------------------------------------------------------
//  Save-file enumeration
//--------------------------------------------------------------

uint16
SaveMgr::GetFirstSave(char *fileName)
{
    if (pSaveDir) {
        closedir(pSaveDir);
        pSaveDir = nullptr;
    }

    pSaveDir = opendir(get_pref_dir());
    getFirstCalled = true;

    if (!pSaveDir) {
        *fileName = '\0';
        return SM_NO_FILE_FOUND;
    }

    return GetNextSave(fileName);
}


uint16
SaveMgr::GetNextSave(char *fileName)
{
    if (!getFirstCalled || !pSaveDir) {
        *fileName = '\0';
        return SM_NO_FILE_FOUND;
    }

    struct dirent *ent;
    while ((ent = readdir(pSaveDir)) != nullptr) {
        size_t len = strlen(ent->d_name);
        if (len > 4 && strcmp(ent->d_name + len - 4, ".sav") == 0) {
            snprintf(fileName, FILENAME_MAX, "%s%s", get_pref_dir(), ent->d_name);
            return SM_NO_ERROR;
        }
    }

    *fileName = '\0';
    return SM_NO_FILE_FOUND;
}


uint16
SaveMgr::GetSaveInfo(char *fileName, char *saveName, uint16 maxNameSize,
                uint16 *versionNum, uint16 *versionSubNum)
{
    FILE   *pFile = fopen(fileName, "r");
    if (pFile == nullptr) return SM_FILE_OPEN_ERROR;

    uint16 result = read_save_header(pFile, saveName, maxNameSize, versionNum);
    if (versionSubNum) *versionSubNum = 0;

    fclose(pFile);
    return result;
}
