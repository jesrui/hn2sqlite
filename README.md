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
   queries used to retrieve stories by date, comments of a specific story and
   stories or comments submitted by an specific author. Generating the indexes
   takes a while too.

        $ sqlite3 hn-all-stories+comments.sqlite
        SQLite version 3.8.5 2014-06-04 14:06:34
        Enter ".help" for usage hints.
        sqlite> .timer on
        sqlite> create index if not exists created_at_i_idx on stories (created_at_i);
        Run Time: real 50.425 user 4.280000 sys 4.333333
        sqlite> create index if not exists story_id_idx on comments (story_id);
        Run Time: real 212.497 user 19.619998 sys 18.913332
        sqlite> create index if not exists stories_author_idx on stories (author);
        Run Time: real 53.204 user 6.210000 sys 4.396666
        sqlite> create index if not exists comments_author_idx on comments (author);
        Run Time: real 353.158 user 30.969997 sys 30.193330
        sqlite> .quit

The resulting database weights 3.8 GBytes. SQLite shows no problem dealing with
that.

## Hacker News Digest

Check out [Hacker News Digest](https://github.com/jesrui/HackerNewsDigest), a
companion web app to browse the data.

## Acknowledgemnts

* [rapidjson](https://github.com/pah/rapidjson) is used to parse the input
files.

## License

`hn2sqlite` is distributed under the
[MIT license](http://opensource.org/licenses/MIT).
