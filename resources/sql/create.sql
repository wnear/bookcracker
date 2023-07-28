
CREATE TABLE IF NOT EXISTS localchannel (
    id INTEGER unique,
    title TEXT,
    feedurl TEXT,
    description TEXT,

    iconUrl TEXT,
    websiteUrl TEXT,
    date_created DATETIME,
    valid BOOLEAN,
    pullOK INTEGER,
    pullFail INTEGER
);


-- all the fields contained in the xml file.
CREATE TABLE IF NOT EXISTS episodes (
    id INTEGER unique,
    title TEXT,
    mediafileUrl TEXT,
    channelid INTEGER REFERENCES localchannel(id),
    description TEXT,

    mediaSize  INTEGER,
    mediaDuration INTEGER,
    filesize INTEGER,
    duration INTEGER,

    iconUrl TEXT,
    websiteUrl TEXT,
    webpageUrl TEXT,
    date_created DATETIME,
    date_published DATETIME,
    valid BOOLEAN,

    cached BOOLEAN,
    cacheLocation TEXT,

    pullOK INTEGER,
    pullFail INTEGER
);


-- CREATE TABLE IF NOT EXISTS channel-config ();
-- updatetime.

-- CREATE TABLE IF NOT EXISTS episodes-config ();
-- notes: favourite. stars. lock for no delete, etc.

-- CREATE TABLE IF NOT EXISTS playhistory ();


-- CREATE TABLE IF NOT EXISTS notes ();


