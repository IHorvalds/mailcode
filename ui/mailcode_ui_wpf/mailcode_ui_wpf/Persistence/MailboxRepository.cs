using Microsoft.Data.Sqlite;
using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;

namespace mailcode_ui_wpf
{
    public class MailboxRepository
    {
        private readonly SqliteConnectionStringBuilder connectionStringBuilder;
        private readonly SqliteConnection connection;
        private string _emailFilter = string.Empty;

        private static MailboxRepository? _repository;

        private readonly ObservableCollection<Mailbox> _mailboxes = new();

        public static MailboxRepository Instance()
        {
            if (_repository == null)
            {
                string baseDirectory = AppDomain.CurrentDomain.BaseDirectory;
                string databasePath = Path.Combine(baseDirectory, "Data", "watched-emails.db");
                _repository = new MailboxRepository(databasePath);
            }

            return _repository!;
        }

        public ObservableCollection<Mailbox> GetMailboxes()
        {
            return _mailboxes; 
        }

        private MailboxRepository(string databasePath)
        {
            if (!File.Exists(databasePath))
            {
                Directory.CreateDirectory(path: Path.GetDirectoryName(databasePath)!);
                var dbFile = File.OpenWrite(databasePath);
                dbFile.Close();
            }

            connectionStringBuilder = new SqliteConnectionStringBuilder
            {
                DataSource = databasePath,
                Mode = SqliteOpenMode.ReadWriteCreate
            };
            connection = new SqliteConnection(connectionStringBuilder.ConnectionString);
            EnsureDatabaseCreated();
            RefreshMailboxList();
        }

        ~MailboxRepository()
        {
            connection.Close();
        }

        private void EnsureDatabaseCreated()
        {
            connection.Open();

            var createTableCommand = connection.CreateCommand();
            createTableCommand.CommandText = "CREATE TABLE IF NOT EXISTS Mailboxes (" +
                "Email TEXT PRIMARY_KEY, " +
                "Password TEXT NOT NULL," +
                "ServerUrl TEXT NOT NULL, " +
                "Port INTEGER NOT NULL, " +
                "UseSSL INTEGER NOT NULL)";

            createTableCommand.ExecuteNonQuery();
        }

        public void SetEmailFilter(string email)
        {
            _emailFilter = email;
        }

        public void RefreshMailboxList()
        {
            connection.Open();

            var selectCommand = connection.CreateCommand();
            selectCommand.CommandText = "SELECT * FROM Mailboxes";

            if (_emailFilter != string.Empty)
            {
                selectCommand.CommandText += " WHERE Email LIKE @EmailFilter";
                selectCommand.Parameters.AddWithValue("@EmailFilter", "%" + _emailFilter + "%");
            }

            _mailboxes.Clear();
            using var reader = selectCommand.ExecuteReader();
            while (reader.Read())
            {
                var mailbox = new Mailbox
                (
                    reader.GetString(0),
                    reader.GetString(1),
                    reader.GetString(2),
                    (uint)reader.GetInt32(3),
                    reader.GetBoolean(4)
                );
                _mailboxes.Add(mailbox);
            }
        }

        public bool AddMailbox(Mailbox mailbox)
        {
            var insertCommand = connection.CreateCommand();
            insertCommand.CommandText = "INSERT INTO Mailboxes (Email, Password, ServerUrl, Port, UseSSL) " +
                "VALUES (@Email, @Password, @ServerUrl, @Port, @UseSSL)";
            insertCommand.Parameters.AddWithValue("@Email", mailbox.Email);
            insertCommand.Parameters.AddWithValue("@Password", mailbox.Password);
            insertCommand.Parameters.AddWithValue("@ServerUrl", mailbox.ServerUrl);
            insertCommand.Parameters.AddWithValue("@Port", mailbox.Port);
            insertCommand.Parameters.AddWithValue("@UseSSL", mailbox.UseSSL);

            int added = insertCommand.ExecuteNonQuery();

            if (added != 1)
            {
                // Log error
                return false;
            }

            _mailboxes.Add(mailbox);
            return true;
        }

        public bool UpdateMailbox(Mailbox mailbox)
        {
            var updateCommand = connection.CreateCommand();
            updateCommand.CommandText = "UPDATE Mailboxes " +
                "SET Password = @Password, ServerUrl = @ServerUrl, Port = @Port, UseSSL = @UseSSL " +
                "WHERE EMAIL = @Email";
            updateCommand.Parameters.AddWithValue("@Email", mailbox.Email);
            updateCommand.Parameters.AddWithValue("@Password", mailbox.Password);
            updateCommand.Parameters.AddWithValue("@ServerUrl", mailbox.ServerUrl);
            updateCommand.Parameters.AddWithValue("@Port", mailbox.Port);
            updateCommand.Parameters.AddWithValue("@UseSSL", mailbox.UseSSL);

            int updatedRows = updateCommand.ExecuteNonQuery();

            if (updatedRows == 0)
            {
                // log error. No rows updated
                return false;
            }

            for (int i = 0; i < _mailboxes.Count; ++i)
            {
                if (_mailboxes[i].Email == mailbox.Email)
                {
                    _mailboxes[i].CopyFrom(mailbox);
                    --updatedRows;
                }
            }

            if (updatedRows != 0)
            {
                // log error. Inconsistent state. Refresh data
                return false;
            }

            return true;
        }

        public bool DeleteMailbox(string email)
        {
            var deleteCommand = connection.CreateCommand();
            deleteCommand.CommandText = "DELETE FROM Mailboxes WHERE Email = @Email";
            deleteCommand.Parameters.AddWithValue("@Email", email);

            int deletedRows = deleteCommand.ExecuteNonQuery();

            if (deletedRows == 0)
            {
                // log error. No rows deleted
                return false;
            }

            for (int i = 0; i < _mailboxes.Count; ++i)
            {
                if (_mailboxes[i].Email == email)
                {
                    _mailboxes.RemoveAt(i);
                    --deletedRows;
                }
            }

            if (deletedRows != 0)
            {
                // log error. Inconsistent state. Refresh data
                return false;
            }

            return true;
        }

        public Mailbox? GetMailboxByEmail(string email)
        {
            var selectCommand = connection.CreateCommand();
            selectCommand.CommandText = "SELECT * FROM Mailboxes WHERE Email = @Email";
            selectCommand.Parameters.AddWithValue("@Email", email);

            using var reader = selectCommand.ExecuteReader();
            if (reader.Read())
            {
                var mailbox = new Mailbox
                (
                    reader.GetString(0),
                    reader.GetString(1),
                    reader.GetString(2),
                    (uint)reader.GetInt32(3),
                    reader.GetBoolean(4)
                );
                return mailbox;
            }

            return null;
        }
    }
}