
-- word is known or what.
CREATE TABLE IF NOT EXISTS word_info (
    -- id INTEGER,
    --AUTOINCREMENT,
    word TEXT unique primary key,

    proficiency INTEGER
    --  1 known
    --  2 in dict, learning
    --  3 ignore.
);

-- as a dict cache.
CREATE TABLE IF NOT EXISTS word_definition (
    -- id INTEGER,
    word TEXT unique primary key,

    meaning TEXT,

    valid_literally BOOLEAN,
    level_is_init BOOLEAN,
    is_middleSchool BOOLEAN,
    is_highSchool BOOLEAN,
    is_cet4 BOOLEAN,
    is_cet6 BOOLEAN,
    is_pro4 BOOLEAN,
    is_pro8 BOOLEAN,
    is_tofel BOOLEAN,
    is_gre BOOLEAN,
    is_graduration BOOLEAN
);


-- CREATE UNIQUE INDEX word_info_word_idx ON word_info (word);

CREATE TABLE IF NOT EXISTS word_context (
    -- id INTEGER,
    word TEXT primary key,
    sentence TEXT,
    pos_page INTEGER,
    pos_book TEXT,
    pos_bookauthor TEXT
);

-- CREATE UNIQUE INDEX word_context_word_idx ON word_context (word);

CREATE TABLE IF NOT EXISTS reading_history (
    id INTEGER unique,
    book TEXT,
    author TEXT,
    location TEXT,
    lastTime DATETIME
);



-- all the fields contained in the xml file.

-- CREATE TABLE IF NOT EXISTS episodes (
--     id INTEGER unique,
--     title TEXT,
--     mediafileUrl TEXT,
--     channelid INTEGER REFERENCES localchannel(id),
--     description TEXT,

--     mediaSize  INTEGER,
--     mediaDuration INTEGER,
--     filesize INTEGER,
--     duration INTEGER,

--     iconUrl TEXT,
--     websiteUrl TEXT,
--     webpageUrl TEXT,
--     date_created DATETIME,
--     date_published DATETIME,
--     valid BOOLEAN,

--     cached BOOLEAN,
--     cacheLocation TEXT,

--     pullOK INTEGER,
--     pullFail INTEGER
-- );


-- CREATE TABLE IF NOT EXISTS channel-config ();
-- updatetime.

-- CREATE TABLE IF NOT EXISTS episodes-config ();
-- notes: favourite. stars. lock for no delete, etc.

-- CREATE TABLE IF NOT EXISTS playhistory ();


-- CREATE TABLE IF NOT EXISTS notes ();


