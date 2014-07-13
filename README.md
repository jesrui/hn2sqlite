# hn2sqlite

`hn2sqlite` is a tool to dump [Hacker News](https://news.ycombinator.com)
stories and comments in a SQLite database. It reads
[HackerNewsData](https://github.com/sytelus/HackerNewsData) in JSON format and
outputs the SQLite database. See [Downloading All of Hacker News Posts and
Comments](https://news.ycombinator.com/item?id=7835605).

## How to generate the database

1. build the `hn2sqlite` tool

        cd ~/hn2sqlite; make

1. Download and unpack the data dump. I had more luck with the copy at the
   [internet archive](https://archive.org/details/HackerNewsStoriesAndCommentsDump)
   than with the [torrent](https://github.com/sytelus/HackerNewsData) at GitHub.

1. Import all the stories in the database. This takes about one minute on my
   computer.

        ./hn2sqlite hn-all-stories+comments.sqlite < HNStoriesAll.json

1. Import all the comments. This takes about 5 min.

        ./hn2sqlite hn-all-stories+comments.sqlite < HNCommentsAll.json

1. Create the following database indexes. These indexes work ok for me with
   queries used to retrieve stories by date and the comments of a specific
   story. Generating the indexes takes a while too.

        $ sqlite3 hn-all-stories+comments.sqlite \
            "create index if not exists created_at_i_idx on stories (created_at_i)"
        $ sqlite3 hn-all-stories+comments.sqlite \
            "create index if not exists story_id_idx on comments (story_id)"

## Sample SQL queries

The resulting database weights 3.7 GBytes. As the following sample queries show,
SQLite has no problem with that size:

    $ # Fetch stories between 12:30 and 13:00 UTC on the 2013-12-31
    $ export TZ= # to get UTC times
    $ t1=$(date --date=2013-12-31T12:30 +%s)
    $ t2=$(date --date=2013-12-31T13:00 +%s)
    $ q="select objectID, created_at_i, author, num_comments, points, title \
        from stories where created_at_i between $t1 and $t2"
    $ time sqlite3 hn-all-stories+comments.sqlite "$q"
    ...
    6990218|1388494556|apaprocki|4|29|Lost Images Come To Life A Century After Antarctic Expedition
    ...

    real	0m0.006s
    user	0m0.003s
    sys	0m0.000s

    $ # Fetch all comments of one of those stories
    $ sqlite3 hn-all-stories+comments.sqlite
    SQLite version 3.8.5 2014-06-04 14:06:34
    Enter ".help" for usage hints.
    sqlite> .timer on
    sqlite> select objectID, created_at_i, author from comments where story_id = 6990218 order by created_at_i desc;
    6991900|1388513705|dded
    6991515|1388509971|justincormack
    6990712|1388501882|yanivs
    6990574|1388500322|arethuza
    Run Time: real 0.072 user 0.000000 sys 0.003333

## Hacker News Digest

For a more convenient usage I have written
[Hacker News Digest](https://github.com/jesrui/HackerNewsDigest), a companion
web app to browse the data.

## Acknowledgemnts

* [rapidjson](https://github.com/pah/rapidjson) is used to parse the input
files.

## License

`hn2sqlite` is distributed under the
[MIT license](http://opensource.org/licenses/MIT).
