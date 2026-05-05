#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTest>

class AuthTest;
class UserTest;
class FriendTest;
class MessageTest;
class FlowTest;

int runAuthTest(int argc, char** argv);
int runUserTest(int argc, char** argv);
int runFriendTest(int argc, char** argv);
int runMessageTest(int argc, char** argv);
int runFileTest(int argc, char** argv);
int runGroupTest(int argc, char** argv);
int runFlowTest(int argc, char** argv);

namespace {

int runToFile(int (*runner)(int, char**), const QString& outputPath)
{
    QByteArray option("-o");
    QByteArray output = outputPath.toLocal8Bit() + QByteArray(",txt");
    QByteArray appName("im_api_tests");

    char* args[] = {
        appName.data(),
        option.data(),
        output.data()
    };
    return runner(3, args);
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    QDir().mkpath(QStringLiteral("tests/results"));

    int status = 0;
    status |= runToFile(runAuthTest, QStringLiteral("tests/results/AuthTest.txt"));
    status |= runToFile(runUserTest, QStringLiteral("tests/results/UserTest.txt"));
    status |= runToFile(runFriendTest, QStringLiteral("tests/results/FriendTest.txt"));
    status |= runToFile(runMessageTest, QStringLiteral("tests/results/MessageTest.txt"));
    status |= runToFile(runFileTest, QStringLiteral("tests/results/FileTest.txt"));
    status |= runToFile(runGroupTest, QStringLiteral("tests/results/GroupTest.txt"));
    status |= runToFile(runFlowTest, QStringLiteral("tests/results/FlowTest.txt"));
    return status;
}
