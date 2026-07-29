-- Kung Fu Chess — PostgreSQL schema
--
-- Apply manually or via database/migrations/001_initial_schema.sql.
-- Do not rely on application code to run migrations in production.

-- User accounts (players table; rating column stores ELO)
CREATE TABLE IF NOT EXISTS players (
    id SERIAL PRIMARY KEY,
    username TEXT NOT NULL UNIQUE,
    password_hash TEXT,
    rating INTEGER NOT NULL DEFAULT 1000,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Match records (status holds result lifecycle; winner_id set when finished)
CREATE TABLE IF NOT EXISTS games (
    id SERIAL PRIMARY KEY,
    white_player_id INTEGER NOT NULL REFERENCES players(id),
    black_player_id INTEGER NOT NULL REFERENCES players(id),
    winner_id INTEGER REFERENCES players(id),
    status TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    finished_at TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS idx_games_white_player_id ON games(white_player_id);
CREATE INDEX IF NOT EXISTS idx_games_black_player_id ON games(black_player_id);
CREATE INDEX IF NOT EXISTS idx_games_status ON games(status);
