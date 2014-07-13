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

The resulting database weights 3.7 GBytes. SQLite shows no problem dealing with
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
