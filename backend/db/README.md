# Backend Database

The backend database is a MySQL database that is used to cache the translations and audio files. This way, we can avoid making too many requests to the LLM.

## Prerequisites

`brew install mysql-connector-c++`

## Creation

1. Log in as root
   
```sql
mysql -h 127.0.0.1 -u root -p
```

2. Create the database

```sql
-- Create the database if it doesn't exist
CREATE DATABASE IF NOT EXISTS hansnap_db;

-- Create the user with password (replace 'your_password' with actual password)
CREATE USER 'hansnap_user'@'localhost' IDENTIFIED BY 'your_password';

-- Grant privileges to the user on the database
GRANT ALL PRIVILEGES ON hansnap_db.* TO 'hansnap_user'@'localhost';

-- Apply the changes
FLUSH PRIVILEGES;

-- Exit MySQL
EXIT;
```

3. Run the migrations

```
make down && make up
```

4. Verify the tables

```sql
mysql -h 127.0.0.1 -u hansnap_user -p hansnap_db -e "SHOW TABLES;"
```

### Testing

```sql
CREATE USER 'hansnap_user'@'localhost' IDENTIFIED BY 'your_password';
GRANT ALL PRIVILEGES ON hansnap_test_db.* TO 'hansnap_user'@'localhost';
GRANT CREATE, DROP ON *.* TO 'hansnap_user'@'localhost';
FLUSH PRIVILEGES;
```

### Translations

### Audio Files
