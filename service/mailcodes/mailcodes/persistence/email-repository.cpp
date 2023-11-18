// #include <sqlite3.h>
#include <SQLiteCpp/SQLiteCpp.h>

#include "module.h"
#include "email-repository.h"
#include "common/paths.h"

namespace Persistence
{

//! Email Cursor
EmailDBRepository::EmailCursor::EmailCursor(SQLite::Statement&& rvQuery, size_t rowCount)
    : mpQuery(std::make_unique<SQLite::Statement>(std::move(rvQuery))), mRowCount(rowCount)
{
}

EmailDBRepository::EmailCursor::EmailCursor(EmailCursor&& other) noexcept
{
    mpQuery.swap(other.mpQuery);
    mRowCount = other.mRowCount;

    other.mpQuery.reset();
    other.mRowCount = 0;
}

EmailDBRepository::EmailCursor& EmailDBRepository::EmailCursor::operator=(EmailCursor&& other) noexcept
{
    mpQuery.swap(other.mpQuery);
    mRowCount = other.mRowCount;

    other.mpQuery.reset();
    other.mRowCount = 0;
    return *this;
}

bool EmailDBRepository::EmailCursor::hasMore()
{
    return !mpQuery->isDone();
}

bool EmailDBRepository::EmailCursor::next(Email& rEmail)
{
    ENTERED();
    bool result = true;

    try
    {
        result = mpQuery->executeStep();

        rEmail.email     = mpQuery->getColumn("email").getString();
        rEmail.password  = mpQuery->getColumn("password").getString();
        rEmail.serverUrl = mpQuery->getColumn("serverUrl").getString();
        rEmail.port      = mpQuery->getColumn("port");
        rEmail.useSSL    = mpQuery->getColumn("useSSL").getInt();

        auto connectionFailureColumn = mpQuery->getColumn("connectionFailure");
        if (connectionFailureColumn.isNull())
        {
            rEmail.connectionFailure = std::nullopt;
        }
        else
        {
            rEmail.connectionFailure = connectionFailureColumn.getString();
        }
    }
    catch (const SQLite::Exception& roException)
    {
        LOGGER().error(roException.what());
        result = false;
    }

    FINISHED(result ? "Successful" : "Failed");
    return result;
}

size_t EmailDBRepository::EmailCursor::getRowCount() const
{
    return mRowCount;
}

//! Email DB Repository
bool EmailDBRepository::initialize(std::string_view svFilename)
{
    ENTERED();
    bool result = true;

    try
    {
        mDatabase = SQLite::Database(svFilename.data(), SQLite::OPEN_READWRITE);

        LOGGER().info("Opened database {} successfully", svFilename);

        result = mDatabase.tableExists(Email::tableName());

        if (!result)
        {
            LOGGER().warn("\"{}\" table does not exist. Run the UI to create it.", Email::tableName());
        }
    }
    catch (const SQLite::Exception& roException)
    {
        LOGGER().error(roException.what());
        result = false;
    }

    FINISHED("initialized", result);
    return result;
}

EmailDBRepository::EmailCursor EmailDBRepository::getAll()
{
    ENTERED();
    bool result = true;

    SQLite::Statement query(mDatabase, std::format("SELECT * FROM {}", Email::tableName()));
    size_t            rowCount = 0;

    try
    {
        rowCount = mDatabase.execAndGet(std::format("SELECT COUNT(*) FROM {}", Email::tableName())).getInt();
    }
    catch (const SQLite::Exception& rException)
    {
        LOGGER().error(rException.what());
        result = false;
    }

    FINISHED(result ? std::format("Found {} emails", rowCount) : "Failed to get emails from database");
    return std::move(EmailCursor(std::move(query), rowCount));
}

std::optional<Email> EmailDBRepository::get(std::string_view svEmailAddrress)
{
    ENTERED();
    std::optional<Email> maybeEmail = std::nullopt;

    try
    {
        SQLite::Statement query(mDatabase,
                                std::format("SELECT * FROM {} WHERE {} LIKE :email", Email::tableName(), "email"));
        query.bind(std::string(":email"), std::format("%{}%", svEmailAddrress));

        if (query.executeStep())
        {
            maybeEmail = Email {.email     = query.getColumn("email").getString(),
                                .password  = query.getColumn("password").getString(),
                                .serverUrl = query.getColumn("serverUrl").getString(),
                                .port      = query.getColumn("port"),
                                .useSSL    = (bool) query.getColumn("useSSL").getInt(),

                                .connectionFailure = [&query]() -> std::optional<std::string> {
                                    SQLite::Column col = query.getColumn("connectionFailure");
                                    if (col.isNull())
                                        return std::nullopt;

                                    return col.getString();
                                }()};
        }
    }
    catch (const SQLite::Exception& rException)
    {
        LOGGER().error(rException.what());
    }

    FINISHED(maybeEmail.has_value());
    return maybeEmail;
}

bool EmailDBRepository::updateConnectionFailure(const std::string&                rsEmailAddress,
                                                const std::optional<std::string>& rsFailureReason)
{
    ENTERED();
    bool result = true;

    try
    {
        SQLite::Statement update(mDatabase, "UPDATE {} SET {}=:connectionFailure WHERE email=:email;"); // TODO
        update.bind(":email", rsEmailAddress);
        if (rsFailureReason.has_value())
        {
            update.bind(":connectionFailure", rsFailureReason.value());
        }
        else
        {
            update.bind(":connectionFailure");
        }

        int affectedRows = update.exec();

        LOGGER().info("Updated connection failure reason for {} to {}", rsEmailAddress,
                      rsFailureReason.value_or("\"none\""));
    }
    catch (const SQLite::Exception& rException)
    {
        LOGGER().error(rException.what());
        result = false;
    }

    FINISHED(std::format("{} email {}", result ? "Successfully updated" : "Failed to update", rsEmailAddress));
    return result;
}

} // namespace Persistence