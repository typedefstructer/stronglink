#ifndef EARTHFS_H
#define EARTHFS_H

#include "db/db_base.h"
#include "async/async.h"
#include "common.h"

#define URI_MAX 1024

typedef struct SLNRepo* SLNRepoRef;
typedef struct SLNSession* SLNSessionRef;
typedef struct SLNSubmission* SLNSubmissionRef;
typedef struct SLNHasher* SLNHasherRef;
typedef struct SLNFilter* SLNFilterRef;
typedef struct SLNJSONFilterParser* SLNJSONFilterParserRef;
typedef struct SLNPull* SLNPullRef;

SLNRepoRef SLNRepoCreate(strarg_t const dir, strarg_t const name);
void SLNRepoFree(SLNRepoRef *const repoptr);
strarg_t SLNRepoGetDir(SLNRepoRef const repo);
strarg_t SLNRepoGetDataDir(SLNRepoRef const repo);
str_t *SLNRepoCopyInternalPath(SLNRepoRef const repo, strarg_t const internalHash);
strarg_t SLNRepoGetTempDir(SLNRepoRef const repo);
str_t *SLNRepoCopyTempPath(SLNRepoRef const repo);
strarg_t SLNRepoGetCacheDir(SLNRepoRef const repo);
strarg_t SLNRepoGetName(SLNRepoRef const repo);
void SLNRepoDBOpen(SLNRepoRef const repo, DB_env **const dbptr);
void SLNRepoDBClose(SLNRepoRef const repo, DB_env **const dbptr);
void SLNRepoSubmissionEmit(SLNRepoRef const repo, uint64_t const sortID);
int SLNRepoSubmissionWait(SLNRepoRef const repo, uint64_t const sortID, uint64_t const future);
void SLNRepoPullsStart(SLNRepoRef const repo);
void SLNRepoPullsStop(SLNRepoRef const repo);

int SLNRepoCookieCreate(SLNRepoRef const repo, strarg_t const username, strarg_t const password, str_t **const outCookie);
int SLNRepoCookieAuth(SLNRepoRef const repo, strarg_t const cookie, uint64_t *const outUserID);


typedef struct {
	str_t *hash; // Internal hash
	str_t *path;
	str_t *type;
	uint64_t size;
} SLNFileInfo;

SLNSessionRef SLNRepoCreateSession(SLNRepoRef const repo, strarg_t const cookie);
SLNSessionRef SLNRepoCreateSessionInternal(SLNRepoRef const repo, uint64_t const userID); // TODO: Private
void SLNSessionFree(SLNSessionRef *const sessionptr);
SLNRepoRef SLNSessionGetRepo(SLNSessionRef const session);
uint64_t SLNSessionGetUserID(SLNSessionRef const session);
int SLNSessionGetAuthError(SLNSessionRef const session);
str_t **SLNSessionCopyFilteredURIs(SLNSessionRef const session, SLNFilterRef const filter, count_t const max); // TODO: Public API?
int SLNSessionGetFileInfo(SLNSessionRef const session, strarg_t const URI, SLNFileInfo *const info);
void SLNFileInfoCleanup(SLNFileInfo *const info);
int SLNSessionGetValueForField(SLNSessionRef const session, str_t value[], size_t const max, strarg_t const fileURI, strarg_t const field);

SLNSubmissionRef SLNSubmissionCreate(SLNSessionRef const session, strarg_t const type);
void SLNSubmissionFree(SLNSubmissionRef *const subptr);
SLNRepoRef SLNSubmissionGetRepo(SLNSubmissionRef const sub);
strarg_t SLNSubmissionGetType(SLNSubmissionRef const sub);
uv_file SLNSubmissionGetFile(SLNSubmissionRef const sub);
int SLNSubmissionWrite(SLNSubmissionRef const sub, byte_t const *const buf, size_t const len);
int SLNSubmissionEnd(SLNSubmissionRef const sub);
int SLNSubmissionWriteFrom(SLNSubmissionRef const sub, ssize_t (*read)(void *, byte_t const **), void *const context);
strarg_t SLNSubmissionGetPrimaryURI(SLNSubmissionRef const sub);
int SLNSubmissionStore(SLNSubmissionRef const sub, DB_txn *const txn);
// Convenience methods
SLNSubmissionRef SLNSubmissionCreateQuick(SLNSessionRef const session, strarg_t const type, ssize_t (*read)(void *, byte_t const **), void *const context);
int SLNSubmissionBatchStore(SLNSubmissionRef const *const list, count_t const count);

SLNHasherRef SLNHasherCreate(strarg_t const type);
void SLNHasherFree(SLNHasherRef *const hasherptr);
int SLNHasherWrite(SLNHasherRef const hasher, byte_t const *const buf, size_t const len);
str_t **SLNHasherEnd(SLNHasherRef const hasher);
strarg_t SLNHasherGetInternalHash(SLNHasherRef const hasher);

typedef enum {
	SLNFilterTypeInvalid = 0,
	SLNAllFilterType,
	SLNFileTypeFilterType,
	SLNIntersectionFilterType,
	SLNUnionFilterType,
	SLNFulltextFilterType,
	SLNMetadataFilterType,
	SLNLinksToFilterType,
	SLNLinkedFromFilterType,
	SLNPermissionFilterType,
	SLNMetaFileFilterType,
} SLNFilterType;

SLNFilterRef SLNFilterCreate(SLNFilterType const type);
SLNFilterRef SLNPermissionFilterCreate(uint64_t const userID);
void SLNFilterFree(SLNFilterRef *const filterptr);
SLNFilterType SLNFilterGetType(SLNFilterRef const filter);
SLNFilterRef SLNFilterUnwrap(SLNFilterRef const filter);
strarg_t SLNFilterGetStringArg(SLNFilterRef const filter, index_t const i);
int SLNFilterAddStringArg(SLNFilterRef const filter, strarg_t const str, ssize_t const len);
int SLNFilterAddFilterArg(SLNFilterRef const filter, SLNFilterRef const subfilter);
void SLNFilterPrint(SLNFilterRef const filter, count_t const depth);
size_t SLNFilterToUserFilterString(SLNFilterRef const filter, str_t *const data, size_t const size, count_t const depth);
int SLNFilterPrepare(SLNFilterRef const filter, DB_txn *const txn);
void SLNFilterSeek(SLNFilterRef const filter, int const dir, uint64_t const sortID, uint64_t const fileID);
void SLNFilterCurrent(SLNFilterRef const filter, int const dir, uint64_t *const sortID, uint64_t *const fileID);
void SLNFilterStep(SLNFilterRef const filter, int const dir);
uint64_t SLNFilterAge(SLNFilterRef const filter, uint64_t const sortID, uint64_t const fileID);
str_t *SLNFilterCopyNextURI(SLNFilterRef const filter, int const dir, DB_txn *const txn);

SLNJSONFilterParserRef SLNJSONFilterParserCreate(void);
void SLNJSONFilterParserFree(SLNJSONFilterParserRef *const parserptr);
void SLNJSONFilterParserWrite(SLNJSONFilterParserRef const parser, strarg_t const json, size_t const len);
SLNFilterRef SLNJSONFilterParserEnd(SLNJSONFilterParserRef const parser);
SLNFilterType SLNFilterTypeFromString(strarg_t const type, size_t const len);

SLNFilterRef SLNUserFilterParse(strarg_t const query);

SLNPullRef SLNRepoCreatePull(SLNRepoRef const repo, uint64_t const pullID, uint64_t const userID, strarg_t const host, strarg_t const username, strarg_t const password, strarg_t const cookie, strarg_t const query);
void SLNPullFree(SLNPullRef *const pullptr);
int SLNPullStart(SLNPullRef const pull);
void SLNPullStop(SLNPullRef const pull);

#define SLN_URI_MAX 512 // Otherwise use URI_MAX
#define SLN_INTERNAL_ALGO "sha256"
#define SLN_ALGO_SIZE 32
#define SLN_HASH_SIZE 256
#define SLN_ALGO_FMT "%31[a-zA-Z0-9.-]"
#define SLN_HASH_FMT "%255[a-zA-Z0-9.%_-]"
static int SLNParseURI(strarg_t const URI, str_t *const algo, str_t *const hash) {
	int len = 0;
	algo[0] = '\0';
	hash[0] = '\0';
	sscanf(URI, "hash://" SLN_ALGO_FMT "/" SLN_HASH_FMT "%n", algo, hash, &len);
	if(!algo[0]) return -1;
	if(!hash[0]) return -1;
	if('/' == URI[len]) len++;
	if('\0' != URI[len] && '?' != URI[len]) return -1;
	return 0;
}
static str_t *SLNFormatURI(strarg_t const algo, strarg_t const hash) {
	return aasprintf("hash://%s/%s", algo, hash);
}

#endif