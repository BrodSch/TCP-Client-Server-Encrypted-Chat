import csv, sys, sqlite3

connection = sqlite3.connect("database/logs.db")
db = connection.cursor()
db.execute("PRAGMA foreign_keys = ON;")

filename = sys.argv[1]

with open(filename, "r") as file:
    processed_file = csv.DictReader(file)

    processed_file.fieldnames = [name.strip() for name in processed_file.fieldnames]

    for reader in processed_file:
        db.execute("INSERT OR IGNORE INTO user (username) VALUES (?)", (reader["Sender"],))
        db.execute("SELECT id FROM user WHERE username = ?", (reader["Sender"],))
        sender_id = db.fetchone()[0]

        db.execute("INSERT OR IGNORE INTO user (username) VALUES (?)", (reader["Receiver"],))
        db.execute("SELECT id FROM user WHERE username = ?", (reader["Receiver"],))
        receiver_id = db.fetchone()[0]

        db.execute("INSERT INTO conversation (message) VALUES (?)", (reader["Log"].encode(),))
        conversation_id = db.lastrowid

        db.execute("INSERT INTO chat_log (sender_id, receiver_id, conversation_id) VALUES (?, ?, ?)", (sender_id, receiver_id, conversation_id))

connection.commit()
connection.close()
