/**
 * hn2mbox -- convert HackerNews stories and comments from JSON to a sqlite
 * database
 *
 * See https://github.com/jesrui/hn2sqlite and
 * https://github.com/sytelus/HackerNewsData
 */

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <sqlite3.h>
#include <string>

#include "rapidjson/reader.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"

#define printd(...) fprintf(stderr, __VA_ARGS__)
//#define printd(...)

using namespace std;
using namespace rapidjson;

static const int32_t kDbVersion = 1;
static_assert(kDbVersion != 0, "Database version can't be 0");

// Either a story or a comment
struct Item {
    string created_at;
    string title;
    string url;
    string author;
    int points;
    string story_text;
    string comment_text;
    unsigned num_comments;
    unsigned story_id;
    string story_title;
    string story_url;
    unsigned parent_id;
    unsigned created_at_i;
    unsigned objectID;
};

// the Item field being currently parsed
enum class Element
{
    none,
    created_at,
    title,
    url,
    author,
    points,
    story_text,
    comment_text,
    num_comments,
    story_id,
    story_title,
    story_url,
    parent_id,
    created_at_i,
    objectID,
};

int getDBUserVersion(sqlite3 *db, int32_t *user_version)
{
    struct CB {
        static int callback(void *u, int argc, char **argv,
                            char **azColName)
        {
            auto user_version = (int32_t*)u;
            *user_version = argc >= 0 ? atoi(argv[0]) : -1;
            return 0;
        }
    };

    char *zErrMsg = NULL;
    int rc = sqlite3_exec(db, "PRAGMA user_version;", CB::callback, user_version,
                          &zErrMsg);
    if( rc!=SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    return rc;
}

int setDBUserVersion(sqlite3 *db, int32_t user_version)
{
    char q[100];
    snprintf(q, sizeof q, "PRAGMA user_version= %d", user_version);
    char *zErrMsg = NULL;
    int rc = sqlite3_exec(db, q, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    return rc;
}

int setupDatabase(sqlite3 *db)
{
    int32_t user_version = 0;
    int rc = getDBUserVersion(db, &user_version);
    if (rc != SQLITE_OK)
        return rc;
    fprintf(stderr, "DB user_version=%d\n", user_version);

    if (user_version == kDbVersion) // assume database is already initialized
        return SQLITE_OK;
    if (user_version != 0) { // assume incompatible database schema
        rc = SQLITE_SCHEMA;
        fprintf(stderr, "can't init database: %s\n", sqlite3_errstr(rc));
        return rc;
    }

    rc = setDBUserVersion(db, kDbVersion);
    if (rc != SQLITE_OK)
        return rc;

#if 0
    CREATE TABLE IF NOT EXISTS stories (
        objectID       INTEGER PRIMARY KEY,
        created_at     TEXT,
        title          TEXT,
        url            TEXT,
        author         TEXT,
        points         INTEGER,
        story_text     TEXT,
        comment_text   TEXT,
        num_comments   INTEGER,
        story_id       INTEGER,
        story_title    TEXT,
        story_url      TEXT,
        parent_id      INTEGER,
        created_at_i   INTEGER);
#endif

    const char sql_setup_script[] = R"END_SCRIPT(

CREATE TABLE IF NOT EXISTS stories (
    objectID       INTEGER PRIMARY KEY,
    title          TEXT,
    url            TEXT,
    author         TEXT,
    points         INTEGER,
    story_text     TEXT,
    num_comments   INTEGER,
    created_at_i   INTEGER);

CREATE TABLE IF NOT EXISTS comments (
    objectID       INTEGER PRIMARY KEY,
    author         TEXT,
    points         INTEGER,
    comment_text   TEXT,
    story_id       INTEGER,
    parent_id      INTEGER,
    created_at_i   INTEGER);

)END_SCRIPT";

    char *zErrMsg = NULL;
    rc = sqlite3_exec(db, sql_setup_script, NULL, NULL, &zErrMsg);
    if( rc!=SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return rc;
    }

    return SQLITE_OK;
}

int insertStory(sqlite3 *db, const Item &item)
{
    assert(item.parent_id == 0);

//    printd("%s objectID=%u\n", __func__, item.objectID);

    sqlite3_stmt *stmt;
    const char q[] = R"END_SCRIPT(
insert into stories (
        objectID,
        title,
        url,
        author,
        points,
        story_text,
        num_comments,
        created_at_i)
    values (?1,?2,?3,?4,?5,?6,?7,?8);";
)END_SCRIPT";

    int rc = sqlite3_prepare_v2(db, q, sizeof q, &stmt, NULL);
    if (rc)
        goto out;
    rc = sqlite3_bind_int(stmt, 1, item.objectID);
    if (rc)
        goto out;
    rc = sqlite3_bind_text(stmt, 2, item.title.c_str(), -1, SQLITE_TRANSIENT);
    if (rc)
        goto out;
    rc = sqlite3_bind_text(stmt, 3, item.url.c_str(), -1, SQLITE_TRANSIENT);
    if (rc)
        goto out;
    rc = sqlite3_bind_text(stmt, 4, item.author.c_str(), -1, SQLITE_TRANSIENT);
    if (rc)
        goto out;
    rc = sqlite3_bind_int(stmt, 5, item.points);
    if (rc)
        goto out;
    rc = sqlite3_bind_text(stmt, 6, item.story_text.c_str(), -1, SQLITE_TRANSIENT);
    if (rc)
        goto out;
    rc = sqlite3_bind_int(stmt, 7, item.num_comments);
    if (rc)
        goto out;
    rc = sqlite3_bind_int(stmt, 8, item.created_at_i);
    if (rc)
        goto out;

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
//        fprintf(stderr, "can't exec statement: %s\n", sqlite3_errstr(rc));
        goto out;
    }

    rc = SQLITE_OK;

out:
    sqlite3_finalize(stmt);
    return rc;
}

int insertComment(sqlite3 *db, const Item &item)
{
    assert(item.parent_id != 0);

//    printd("%s objectID=%u\n", __func__, item.objectID);

    sqlite3_stmt *stmt;
    const char q[] = R"END_SCRIPT(
insert into comments (
        objectID,
        author,
        points,
        comment_text,
        story_id,
        parent_id,
        created_at_i)
    values (?1,?2,?3,?4,?5,?6,?7);";
)END_SCRIPT";

    int rc = sqlite3_prepare_v2(db, q, sizeof q, &stmt, NULL);
    if (rc)
        goto out;
    rc = sqlite3_bind_int(stmt, 1, item.objectID);
    if (rc)
        goto out;
    rc = sqlite3_bind_text(stmt, 2, item.author.c_str(), -1, SQLITE_TRANSIENT);
    if (rc)
        goto out;
    rc = sqlite3_bind_int(stmt, 3, item.points);
    if (rc)
        goto out;
    rc = sqlite3_bind_text(stmt, 4, item.comment_text.c_str(), -1, SQLITE_TRANSIENT);
    if (rc)
        goto out;
    rc = sqlite3_bind_int(stmt, 5, item.story_id);
    if (rc)
        goto out;
    rc = sqlite3_bind_int(stmt, 6, item.parent_id);
    if (rc)
        goto out;
    rc = sqlite3_bind_int(stmt, 7, item.created_at_i);
    if (rc)
        goto out;

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
//        fprintf(stderr, "can't exec statement: %s\n", sqlite3_errstr(rc));
        goto out;
    }

    rc = SQLITE_OK;

out:
    sqlite3_finalize(stmt);
    return rc;
}

template<typename Encoding = UTF8<>>
struct ItemsHandler {
    typedef typename Encoding::Ch Ch;

    ItemsHandler(sqlite3 *db)
        : db(db), element(Element::none), parent(noparent), level(0), item {}
    {}

    void Default() {}
    void Null() { element = Element::none; }
    void Bool(bool) { Default(); }
    void Int(int i) { Int64(i); }
    void Uint(unsigned i) { Int64(i); }
    void Int64(int64_t i)
    {
        if (parent == _highlightResult)
            return;
        switch (element) {
        case Element::points:         item.points = i;        break;
        case Element::num_comments:   item.num_comments = i;  break;
        case Element::story_id:       item.story_id = i;      break;
        case Element::parent_id:      item.parent_id = i;     break;
        case Element::created_at_i:   item.created_at_i = i;  break;
        default:                      break;
        }

        element = Element::none;
    }
    void Uint64(uint64_t) { Default(); }
    void Double(double) { Default(); }

    void String(const Ch* str, SizeType length, bool copy)
    {
        if (parent == _highlightResult)
            return;
        if (element == Element::none) {
            if (strcmp("hits", str) == 0)
                parent = hits;
            else if (strcmp("_highlightResult", str) == 0)
                parent = _highlightResult;
            else if (strcmp("created_at", str) == 0)
                element = Element::created_at;
            else if (strcmp("title", str) == 0)
                element = Element::title;
            else if (strcmp("url", str) == 0)
                element = Element::url;
            else if (strcmp("author", str) == 0)
                element = Element::author;
            else if (strcmp("points", str) == 0)
                element = Element::points;
            else if (strcmp("story_text", str) == 0)
                element = Element::story_text;
            else if (strcmp("comment_text", str) == 0)
                element = Element::comment_text;
            else if (strcmp("num_comments", str) == 0)
                element = Element::num_comments;
            else if (strcmp("story_id", str) == 0)
                element = Element::story_id;
            else if (strcmp("story_title", str) == 0)
                element = Element::story_title;
            else if (strcmp("story_url", str) == 0)
                element = Element::story_url;
            else if (strcmp("parent_id", str) == 0)
                element = Element::parent_id;
            else if (strcmp("created_at_i", str) == 0)
                element = Element::created_at_i;
            else if (strcmp("objectID", str) == 0)
                element = Element::objectID;
            return;
        }

        if (element == Element::none)
            return;

        // minimal string normalization
        string s = str;
        replace(s.begin(), s.end(), '\n', ' ');
        remove(s.begin(), s.end(), '\r');

        switch (element) {
        case Element::created_at:     item.created_at = s;    break;
        case Element::title:          item.title = s;         break;
        case Element::url:            item.url = s;           break;
        case Element::author:         item.author = s;        break;
        case Element::story_text:     item.story_text = s;    break;
        case Element::comment_text:   item.comment_text = s;  break;
        case Element::story_title:    item.story_title = s;   break;
        case Element::story_url:      item.story_url = s;     break;
        case Element::objectID:       item.objectID = atoi(s.c_str()); break;
        default:                      break;
        }

        element = Element::none;
    }
    void StartObject() { ++level; }
    void EndObject(SizeType)
    {
        if (--level == 1) {
            parent = hits;
            int rc = item.parent_id != 0
                ? insertComment(db, item): insertStory(db, item);
            if (rc)
                fprintf(stderr, "Can't insert objectID=%u: %s\n",
                    item.objectID, sqlite3_errmsg(db));
            item = {};
        }
    }
    void StartArray() { Default(); }
    void EndArray(SizeType) { Default(); }

    sqlite3 *db;
    Element element;
    enum {
        noparent,
        hits,
        _highlightResult,
    } parent;
    int level;

    Item item;
};

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "no database specified\n");
        exit(1);
    }

    sqlite3 *db;
    int rc = sqlite3_open(argv[1], &db);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    rc = setupDatabase(db);
    if (rc) {
        sqlite3_close(db);
        return 1;
    }

    Reader reader;
    char readBuffer[65536];
    FileReadStream is(stdin, readBuffer, sizeof(readBuffer));

    ItemsHandler<> handler(db);

    char *zErrMsg = NULL;
    rc = sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
    if (rc) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 1;
    }

    bool ok = reader.Parse<kParseValidateEncodingFlag>(is, handler);

    if (!ok) {
        fprintf(stderr, "\nError(%u): %s\n",
                (unsigned)reader.GetErrorOffset(), reader.GetParseError());
    }

    rc = sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
    if (rc) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_close(db);

    return ok ? 0 : 1;
}
