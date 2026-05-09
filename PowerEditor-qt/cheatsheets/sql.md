# SQL Cheatsheet

## Básico
```sql
SELECT col1, col2 FROM tbl WHERE cond ORDER BY col DESC LIMIT 10;
SELECT DISTINCT col FROM tbl;
SELECT COUNT(*), AVG(col), MIN(col), MAX(col), SUM(col) FROM tbl;
SELECT col, COUNT(*) FROM tbl GROUP BY col HAVING COUNT(*) > 1;
```

## Joins
```sql
SELECT a.*, b.name
FROM a
INNER JOIN b ON b.a_id = a.id;       -- só matches

SELECT *
FROM a LEFT JOIN b ON b.a_id = a.id; -- todos de A

FROM a RIGHT JOIN b ...              -- todos de B
FROM a FULL OUTER JOIN b ...         -- união
FROM a CROSS JOIN b                  -- produto cartesiano
```

## Subqueries
```sql
SELECT * FROM users
WHERE id IN (SELECT user_id FROM orders WHERE total > 100);

SELECT u.*, (SELECT COUNT(*) FROM orders o WHERE o.user_id = u.id) AS n_orders
FROM users u;
```

## CTE / WITH
```sql
WITH active_users AS (
    SELECT * FROM users WHERE last_login > NOW() - INTERVAL '30 days'
), big_orders AS (
    SELECT * FROM orders WHERE total > 100
)
SELECT au.name, COUNT(bo.id)
FROM active_users au
LEFT JOIN big_orders bo ON bo.user_id = au.id
GROUP BY au.name;

-- Recursive
WITH RECURSIVE tree AS (
    SELECT id, parent_id, name, 1 AS depth FROM nodes WHERE parent_id IS NULL
    UNION ALL
    SELECT n.id, n.parent_id, n.name, t.depth + 1
    FROM nodes n JOIN tree t ON n.parent_id = t.id
)
SELECT * FROM tree;
```

## Window functions
```sql
SELECT
    name,
    salary,
    RANK()        OVER (PARTITION BY dept ORDER BY salary DESC) AS rk,
    ROW_NUMBER()  OVER (PARTITION BY dept ORDER BY salary DESC) AS rn,
    LAG(salary)   OVER (PARTITION BY dept ORDER BY hire_date)   AS prev_salary,
    SUM(salary)   OVER (PARTITION BY dept)                       AS dept_total,
    AVG(salary)   OVER (ORDER BY hire_date ROWS BETWEEN 6 PRECEDING AND CURRENT ROW) AS rolling_avg
FROM employees;
```

## INSERT / UPDATE / DELETE
```sql
INSERT INTO tbl (a, b) VALUES (1, 'x'), (2, 'y');
INSERT INTO tbl (a, b) SELECT a, b FROM other;

UPDATE tbl SET a = a + 1 WHERE id = 5;
UPDATE t SET c = s.c FROM source s WHERE t.id = s.id;  -- PG

DELETE FROM tbl WHERE created_at < NOW() - INTERVAL '1 year';

-- UPSERT (PG)
INSERT INTO tbl (id, val) VALUES (1, 'x')
ON CONFLICT (id) DO UPDATE SET val = EXCLUDED.val;

-- UPSERT (SQLite)
INSERT INTO tbl (id, val) VALUES (1, 'x')
ON CONFLICT(id) DO UPDATE SET val = excluded.val;

-- UPSERT (MySQL)
INSERT INTO tbl (id, val) VALUES (1, 'x')
ON DUPLICATE KEY UPDATE val = VALUES(val);
```

## DDL
```sql
CREATE TABLE users (
    id          BIGSERIAL PRIMARY KEY,
    email       TEXT NOT NULL UNIQUE,
    age         INT CHECK (age >= 0),
    role        TEXT NOT NULL DEFAULT 'user',
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    metadata    JSONB
);

CREATE INDEX idx_users_email ON users(email);
CREATE UNIQUE INDEX idx_users_email_lower ON users(LOWER(email));
CREATE INDEX idx_users_role_created ON users(role, created_at DESC);

ALTER TABLE users ADD COLUMN deleted_at TIMESTAMPTZ;
ALTER TABLE users RENAME COLUMN role TO user_role;
ALTER TABLE users ALTER COLUMN role SET NOT NULL;

DROP TABLE IF EXISTS old_logs;
TRUNCATE TABLE logs;
```

## Foreign keys
```sql
CREATE TABLE orders (
    id      SERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    total   DECIMAL(10,2) NOT NULL
);
```

## Transactions
```sql
BEGIN;
INSERT INTO ...;
UPDATE ...;
COMMIT;     -- ou ROLLBACK;

-- Savepoint (parcial rollback)
SAVEPOINT sp1;
... deu erro ...
ROLLBACK TO sp1;
```

## Performance
```sql
EXPLAIN ANALYZE SELECT ...;
VACUUM ANALYZE;          -- PG: estatísticas + cleanup
ANALYZE tbl;             -- só estatísticas
```

## Strings
```sql
LOWER(s); UPPER(s); INITCAP(s)
LENGTH(s)
SUBSTRING(s FROM 2 FOR 5);
TRIM(s); LTRIM(s); RTRIM(s);
REPLACE(s, 'a', 'b')
CONCAT(a, b)  ou  a || b      -- PG/SQLite
s ILIKE '%foo%'                -- case-insensitive (PG)
s ~ 'regex'                    -- regex (PG)
```

## Datas
```sql
NOW()                       -- timestamp atual
CURRENT_DATE
EXTRACT(YEAR FROM dt)
DATE_TRUNC('month', dt)     -- PG
dt + INTERVAL '7 days'
```
