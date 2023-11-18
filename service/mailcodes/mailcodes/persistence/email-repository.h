#ifndef EMAIL_REPOSITORY_H
#define EMAIL_REPOSITORY_H

#include <string>
#include <filesystem>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <boost/noncopyable.hpp>

namespace Persistence
{

//! @brief Type is mapped to a DB table
struct Email
{
    std::string                email;
    std::string                password;
    std::string                serverUrl;
    int                        port;
    bool                       useSSL = true;
    std::optional<std::string> connectionFailure;

    static constexpr std::string tableName() { return "emails"; }
};

//! @brief The service will only be able to read emails from
//!        the database and to update the connectionFailure
//!        field.
class EmailDBRepository final
{
public:
    //! @brief Cursor for iterating through DB results
    class EmailCursor final : public boost::noncopyable
    {
    public:
        EmailCursor(EmailCursor&& other) noexcept;
        EmailCursor& operator=(EmailCursor&& other) noexcept;

        bool   hasMore();
        bool   next(Email& rEmail);
        size_t getRowCount() const;

    private:
        friend class EmailDBRepository;

        EmailCursor(SQLite::Statement&& rvQuery, size_t rowCount);

        std::unique_ptr<SQLite::Statement> mpQuery;
        size_t                             mRowCount;
    };

    bool initialize(std::string_view svFilename);

    EmailCursor          getAll();
    std::optional<Email> get(std::string_view svEmailAddrress);
    bool updateConnectionFailure(const std::string& rsEmailAddress, const std::optional<std::string>& rsFailureReason);

private:
    std::filesystem::path mDBFilepath;
    SQLite::Database      mDatabase;
};

} // namespace Persistence

#endif // EMAIL_REPOSITORY_H
