"""Сброс локального пароля владельцем компьютера. Закройте SmartExam перед запуском."""
import sqlite3
from datetime import datetime
from pathlib import Path

root = Path(__file__).resolve().parent.parent
database = root / 'data' / 'smart_exam.db'
if not database.is_file():
    raise SystemExit('База данных не найдена.')
print('Закройте все окна SmartExam. История и задания сохранятся.')
if input('Для сброса пароля введите СБРОС: ').strip() != 'СБРОС':
    raise SystemExit('Сброс отменён.')
backup = root / 'bin' / 'backups' / ('password-reset-' + datetime.now().strftime('%Y%m%d-%H%M%S') + '.db')
backup.parent.mkdir(parents=True, exist_ok=True)
with sqlite3.connect(database) as connection:
    with sqlite3.connect(backup) as copy:
        connection.backup(copy)
    connection.execute('DELETE FROM teacher_credentials')
print('Пароль сброшен. При следующем входе учителя задайте новый.')
print('Резервная копия:', backup)
