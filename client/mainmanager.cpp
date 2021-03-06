#include "mainmanager.h"

#include <QMessageBox>
#include <QDebug>

#include <iostream>

#include "myglobal.h"
#include "myodbc.h"
#include "escape.h"

MainManager::MainManager(const char *ip, const char *port, QObject *parent)
    : QObject(parent),
      sslManager(ip, port)
{
    qDebug() << "The thread id of the main thread is " << QThread::currentThreadId();
    //sslManager
    connect(&sslManager, &SslManager::info, [this](std::string content) {
        QMessageBox::information(&mainWindow, "来自服务器的消息", QString(content.c_str()));
    });
    connect(&sslManager, &SslManager::loginFirst, [this] {
        QMessageBox::information(&mainWindow, "提示", "请先登录");
    });
    //Ignore it
//    connect(&sslManager, &SslManager::alreadyLogined, [this] {
//        QMessageBox::information(this, "提示", "您已登录");
//    });
    connect(&sslManager, &SslManager::usernameTooLong, [this] {
        QMessageBox::information(&dialogSignup, "提示", "昵称过长");
    });
    connect(&sslManager, &SslManager::phoneTooLong, [this] {
        QMessageBox::information(&dialogSignup, "提示", "手机号过长");
    });
    connect(&sslManager, &SslManager::noSuchUser, [this] {
        QMessageBox::information(&winLogin, "提示", "用户不存在");
    });
    connect(&sslManager, &SslManager::wrongPassword, [this] {
        QMessageBox::information(&winLogin, "提示", "密码错误");
    });
    connect(&sslManager, &SslManager::alreadyFriends, [this] {
        QMessageBox::information(&dialogAddFriend, "提示", "已经是好友");
    });
    connect(&sslManager, &SslManager::addFriendSent, [this] {
        QMessageBox::information(&dialogAddFriend, "提示", "申请已发送，请等待对方同意");
    });

    //dialogReconnect and sslManager
    connect(&sslManager, &SslManager::sigErr, &dialogReconnect, &DialogReconnect::ShowErr);
    connect(&dialogReconnect, &DialogReconnect::sigRetry, &sslManager, &SslManager::sslconn);
    connect(&sslManager, &SslManager::sigDone, &dialogReconnect, &DialogReconnect::close);
    //dialogSignup
    connect(&winLogin, &WinLogin::signup, &dialogSignup, &DialogSignup::show);
    connect(&dialogSignup, &DialogSignup::signup, &sslManager, &SslManager::signup);
    connect(&sslManager, &SslManager::signupDone, &dialogSignup, &DialogSignup::signupRes);
    //winLogin
    connect(&winLogin, &WinLogin::login, &sslManager, &SslManager::login);
    connect(&sslManager, &SslManager::loginDone, this, &MainManager::HandleLoginDone);
    connect(this, &MainManager::UserPrivateInfoReq, &sslManager, &SslManager::UserPrivateInfoReq);
    connect(&sslManager, &SslManager::UserPrivateInfoReply, &mainWindow, &MainWindow::UpdatePrivateInfo);
    connect(this, &MainManager::AllFriendsReq, &sslManager, &SslManager::AllFriendsReq);
    connect(&sslManager, &SslManager::Friends, this, &MainManager::HandleFriends);
    connect(this, &MainManager::AllGrpsReq, &sslManager, &SslManager::AllGrps);
    connect(&sslManager, &SslManager::Grps, this, &MainManager::HandleGrps);
    //find by username
    connect(&mainWindow, &MainWindow::sigFindByUsername, &dialogFindByUsername, &DialogFindByUsername::show);
    connect(&dialogFindByUsername, &DialogFindByUsername::FindByUsername, &sslManager, &SslManager::FindByUsername);
    connect(&sslManager, &SslManager::FindByUsernameReply, &mainWindow, &MainWindow::HandleFindByUsernameReply);
    //dialogAddFriend
    connect(&mainWindow, &MainWindow::AddFriend, &dialogAddFriend, &DialogAddFriend::show);
    connect(&dialogAddFriend, &DialogAddFriend::AddFriend, &sslManager, &SslManager::AddFriend);
    connect(&sslManager, &SslManager::addFriendReq, this, &MainManager::HandleAddFriendReq);
    connect(this, &MainManager::replyAddFriend, &sslManager, &SslManager::ReplyAddFriend);
    connect(&sslManager, &SslManager::addFriendReply, this, &MainManager::HandleAddFriendReply);
    connect(this, &MainManager::UserPublicInfoReq, &sslManager, &SslManager::UserPublicInfoReq);
    connect(&sslManager, &SslManager::UserPublicInfoReply, this, &MainManager::HandleUserPublicInfoReply);
    //delete friend
    connect(&mainWindow, &MainWindow::sigDeleteFriend, this, &MainManager::DeleteFriend);
    connect(&mainWindow, &MainWindow::sigDeleteFriend, &sslManager, &SslManager::DeleteFriend);
    connect(&sslManager, &SslManager::BeDeletedFriend, this, &MainManager::DeleteFriend);
    connect(&sslManager, &SslManager::BeDeletedFriend, &mainWindow, &MainWindow::DeleteFriend);
    //private message
    connect(&mainWindow, &MainWindow::SendToUser, &sslManager, &SslManager::SendToUser);
    connect(&sslManager, &SslManager::PrivateMsgTooLong, &mainWindow, &MainWindow::HandlePrivateMsgTooLong);
    connect(&sslManager, &SslManager::PrivateMsgResponse, this, &MainManager::HandlePrivateMsgResponse);
    connect(&sslManager, &SslManager::PrivateMsg, this, &MainManager::HandleReceivedPrivateMsg);
    //group
    connect(&mainWindow, &MainWindow::CreateGroup, &dialogCreateGroup, &DialogCreateGroup::show);
    connect(&dialogCreateGroup, &DialogCreateGroup::CreateGroup, &sslManager, &SslManager::CreateGroup);
    connect(&sslManager, &SslManager::NewGrpWithName, this, &MainManager::HandleNewGrpWithName);
    connect(&sslManager, &SslManager::NewGroup, this, &MainManager::HandleNewGroup);
    connect(&mainWindow, &MainWindow::JoinGroup, &dialogJoinGroup, &DialogJoinGroup::show);
    connect(&dialogJoinGroup, &DialogJoinGroup::JoinGroup, &sslManager, &SslManager::JoinGroup);    //reply is NewGroup
    connect(this, &MainManager::GrpInfoReq, &sslManager, &SslManager::GrpInfoReq);
    connect(&sslManager, &SslManager::GrpInfoReply, this, &MainManager::HandleGrpInfoReply);
    connect(&mainWindow, &MainWindow::ChangeGrpOwner, &sslManager, &SslManager::ChangeGrpOwner);
    connect(&mainWindow, &MainWindow::sigAllGrpMember, &sslManager, &SslManager::AllGrpMember);
    //group message
    connect(&mainWindow, &MainWindow::SendToGroup, &sslManager, &SslManager::SendToGroup);
    connect(&sslManager, &SslManager::GrpMsgResp, this, &MainManager::HandleGrpMsgResp);
    connect(&sslManager, &SslManager::GrpMsg, this, &MainManager::HandleReceivedGrpMsg);
    //moments
    connect(&mainWindow, &MainWindow::sigMoments, &dialogMoments, &DialogMoments::HandleShow);
    connect(&dialogMoments, &DialogMoments::sigEditMoment, &dialogEditMoment, &DialogEditMoment::show);
    connect(&dialogEditMoment, &DialogEditMoment::SendMoment, &sslManager, &SslManager::SendMoment);
    connect(&dialogMoments, &DialogMoments::MomentsReq, &sslManager, &SslManager::MomentsReq);
    connect(&sslManager, &SslManager::Moments, &dialogMoments, &DialogMoments::HandleMoments);
    connect(&dialogMoments, &DialogMoments::Comment, &sslManager, &SslManager::SendComment);
    connect(&dialogMoments, &DialogMoments::CommentsReq, &sslManager, &SslManager::CommentsReq);
    connect(&sslManager, &SslManager::Comments, &dialogMoments, &DialogMoments::HandleComments);
    //personal information
    connect(&mainWindow, &MainWindow::sigPersonalInfo, &sslManager, &SslManager::PersonalInfo);

    sslManager.start();
    winLogin.show();
}

MainManager::~MainManager() {
    myodbcLogout();
    //sslManager.stop_io_service();
    sslManager.quit();
    sslManager.wait();
}

void MainManager::HandleLoginDone(userid_t userid) {
    qDebug() << __PRETTY_FUNCTION__;
    myodbcLogin(("Driver=SQLite3;Database=syncchatclient" + std::to_string(userid) + ".db").c_str());

    if (mainWindow.myid) {
        qDebug() << "Duplicate login done: " << userid;
        return;
    }
    mainWindow.myid = userid;
    if (exec_sql("SELECT username FROM user WHERE userid = " + std::to_string(userid) + ';', true)) {
        return;
    }
    char username[MAX_USERNAME_LEN];
    SQLLEN usernameLen;
    SQLBindCol(hstmt, 1, SQL_C_CHAR, username, sizeof(username), &usernameLen);
    if (SQL_NO_DATA != SQLFetch(hstmt)) {
        if (SQL_NULL_DATA == usernameLen) {
            qDebug() << "Error in" << __PRETTY_FUNCTION__ << ": Null username with userid" << userid;
        } else {
            mainWindow.myUsername = std::string(username, usernameLen);
            usernames[userid] = mainWindow.myUsername;
        }
    } else {
        if (exec_sql("INSERT INTO user(userid) VALUES(" + std::to_string(userid) + ");", true)) {
            return;
        }
    }
    if (close_cursor()) {
        return;
    }
    winLogin.close();
    mainWindow.setWindowTitle(QString((
        mainWindow.myUsername + "(账号:" + std::to_string(mainWindow.myid) + ')'
    ).c_str()));
    mainWindow.show();
    emit UserPrivateInfoReq();

    if (exec_sql("SELECT userid, username FROM friends;", true)) {
        return;
    }
    SQLLEN length;
    SQLBindCol(hstmt, 1, SQL_C_UBIGINT, &userid, sizeof(userid), &length);
    SQLBindCol(hstmt, 2, SQL_C_CHAR, username, sizeof(username), &usernameLen);
    while (SQL_NO_DATA != SQLFetch(hstmt)) {
        assert(SQL_NULL_DATA != length);
        std::string displayName;
        if (SQL_NULL_DATA == usernameLen) {
            emit UserPublicInfoReq(userid);
            displayName = std::to_string(userid);
        } else {
            displayName = std::string(username, usernameLen);
        }
        mainWindow.NewFriend(userid, displayName);
    }
    emit AllFriendsReq();

    if (exec_sql("SELECT grpid, grpname FROM grp;", true)) {
        return;
    }
    grpid_t grpid;
    char grpname[MAX_GROUPNAME_LEN];
    SQLBindCol(hstmt, 1, SQL_C_UBIGINT, &grpid, sizeof(grpid), &length);
    SQLBindCol(hstmt, 2, SQL_C_CHAR, grpname, sizeof(grpname), &usernameLen);
    while (SQL_NO_DATA != SQLFetch(hstmt)) {
        assert(SQL_NULL_DATA != length);
        std::string dispName;
        if (SQL_NULL_DATA == usernameLen) {
            emit GrpInfoReq(grpid);
            dispName = std::to_string(grpid);
        } else {
            dispName = std::string(grpname, usernameLen);
        }
        mainWindow.NewGroup(grpid, grpname);
    }
    emit AllGrpsReq();

    if (exec_sql("SELECT msgid, msgtime, sender, touser, content FROM msg;", true)) {
        return;
    }
    SQLLEN contentLen;
    msgid_t msgid;
    msgtime_t msgtime;
    userid_t sender, touser;
    uint8_t content[MAX_CONTENT_LEN];
    SQLBindCol(hstmt, 1, SQL_C_UBIGINT, &msgid, sizeof(msgid), &length);
    SQLBindCol(hstmt, 2, SQL_C_SBIGINT, &msgtime, sizeof(msgtime), &length);
    SQLBindCol(hstmt, 3, SQL_C_UBIGINT, &sender, sizeof(sender), &length);
    SQLBindCol(hstmt, 4, SQL_C_UBIGINT, &touser, sizeof(touser), &length);
    SQLBindCol(hstmt, 5, SQL_C_BINARY, content, sizeof(content), &contentLen);
    while (SQL_NO_DATA != SQLFetch(hstmt)) {
        qDebug() << "A local message";
        assert(SQL_NULL_DATA != contentLen);
        mainWindow.HandleRawPrivateMsg(msgid, msgtime, sender, touser, CppContent(content, content + contentLen));
    }

    if (exec_sql("SELECT grpmsgid, grpmsgtime, sender, togrp, content FROM grpmsg;", true)) {
        return;
    }
    grpmsgid_t grpmsgid;
    SQLBindCol(hstmt, 1, SQL_C_UBIGINT, &grpmsgid, sizeof(grpmsgid), &length);
    SQLBindCol(hstmt, 2, SQL_C_SBIGINT, &msgtime, sizeof(msgtime), &length);
    SQLBindCol(hstmt, 3, SQL_C_UBIGINT, &sender, sizeof(sender), &length);
    SQLBindCol(hstmt, 4, SQL_C_UBIGINT, &grpid, sizeof(grpid), &length);
    SQLBindCol(hstmt, 5, SQL_C_BINARY, content, sizeof(content), &contentLen);
    while (SQL_NO_DATA != SQLFetch(hstmt)) {
        assert(SQL_NULL_DATA != contentLen);
        mainWindow.HandleGrpMsg(grpmsgid, msgtime, sender, grpid, CppContent(content, content + contentLen));
    }
}

void MainManager::HandleAddFriendReq(userid_t userid, std::string username) {
    std::string msg = "账号为" + std::to_string(userid) + "\n昵称为" + username + "\n同意吗？";
    bool reply = (QMessageBox::Yes == QMessageBox::question(
        NULL, "加好友请求", QString(msg.c_str()), QMessageBox::Yes | QMessageBox::No
    ));
    if (reply) {
        exec_sql("INSERT INTO friends(userid, username) VALUES(" +
                 std::to_string(userid) + ",\"" + escape(username) + "\");", true);
        mainWindow.NewFriend(userid, username);
    }
    emit replyAddFriend(userid, reply);
}

void MainManager::HandleAddFriendReply(userid_t userid, bool reply) {
    if (!reply) {
        QMessageBox::information(NULL, "提示", "用户" + QString(std::to_string(userid).c_str()) + "拒绝了您的好友申请");
        return;
    }
    HandleNewFriend(userid);
    QMessageBox::information(NULL, "提示", "您与" + QString(std::to_string(userid).c_str()) + "成为好友了");
}
void MainManager::HandleNewFriend(userid_t userid) {
    auto it = usernames.find(userid);
    if (usernames.end() == it) {
        emit UserPublicInfoReq(userid);
        exec_sql("INSERT INTO friends(userid) VALUES(" + std::to_string(userid) + ");", true);
        mainWindow.NewFriend(userid, std::to_string(userid));
    } else {
        exec_sql("INSERT INTO friends(userid, username) VALUES(" +
                 std::to_string(userid) + ",\"" + escape(it->second) + "\");", true);
        mainWindow.NewFriend(userid, it->second);
    }
}

void MainManager::HandleUserPublicInfoReply(userid_t userid, std::string username) {
    exec_sql("UPDATE friends SET username = \"" + escape(username) + "\" WHERE userid = " + std::to_string(userid) + ';', true);
    usernames[userid] = username;
    qDebug() << "userid = " << userid << ", username = " << username.c_str();
    mainWindow.UpdateUsername(userid, username);
}

void MainManager::HandleFriends(std::vector<userid_t> friends) {
    //This function is called after login has been done and all friends have been read from db
    for (userid_t id : friends) {
        auto it = mainWindow.userChatInfo.find(id);
        if (mainWindow.userChatInfo.end() != it) {
            //already know
            continue;
        } else {
            //This client know the friend for the first time
            HandleNewFriend(id);
        }
    }
}

void MainManager::DeleteFriend(userid_t userid) {
    if (exec_sql("DELETE FROM friends WHERE userid = " + std::to_string(userid) + ';', true)) {
        qWarning() << "Error in" << __PRETTY_FUNCTION__ << ": exec_sql failed";
    }
}

void MainManager::HandlePrivateMsgResponse(userid_t userid, CppContent content, msgid_t msgid, msgtime_t msgtime) {
    mainWindow.HandlePrivateMsgResponse(userid, content, msgid, msgtime);
    WritePrivateMsgToDB(msgid, msgtime, mainWindow.myid, userid, content);
}

void MainManager::HandleReceivedPrivateMsg(userid_t userid, CppContent content, msgid_t msgid, msgtime_t msgtime) {
    mainWindow.HandlePrivateMsg(userid, userid, content, msgid, msgtime);
    WritePrivateMsgToDB(msgid, msgtime, userid, mainWindow.myid, content);
}

bool MainManager::WritePrivateMsgToDB(msgid_t msgid, msgtime_t msgtime, userid_t sender, userid_t touser, CppContent content) {
    SQLLEN length = content.size();
    SQLRETURN retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY, MAX_CONTENT_LEN,
                     0, content.data(), content.size(), &length);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "Error in" << __PRETTY_FUNCTION__ << ": SQLBindParameter failed\n";
        return true;
    }
    return exec_sql("INSERT INTO msg(msgid, msgtime, sender, touser, content) VALUES(" + std::to_string(msgid) + ',' +
             std::to_string(msgtime) + ',' + std::to_string(sender) + ',' + std::to_string(touser) + ",?);", true);
}

void MainManager::HandleNewGrpWithName(grpid_t grpid, std::string grpname) {
    exec_sql("INSERT INTO grp(grpid, grpname) VALUES(" + std::to_string(grpid) +
             ",\"" + escape(grpname) + "\");", true);
    mainWindow.NewGroup(grpid, grpname);
}

void MainManager::HandleNewGroup(grpid_t grpid) {
    emit GrpInfoReq(grpid);
    exec_sql("INSERT INTO grp(grpid) VALUES(" + std::to_string(grpid) + ");", true);
    mainWindow.NewGroup(grpid, std::to_string(grpid));
}

void MainManager::HandleGrpInfoReply(grpid_t grpid, std::string grpname) {
    exec_sql("UPDATE grp SET grpname = \"" + escape(grpname) + "\" WHERE grpid = " + std::to_string(grpid) + ';', true);
    mainWindow.UpdateGrpname(grpid, grpname);
}

void MainManager::HandleGrps(std::vector<grpid_t> grps) {
    //This function is called after login has been done and all groups have been read from db
    for (grpid_t grp : grps) {
        auto it = mainWindow.grpChatInfo.find(grp);
        if (mainWindow.grpChatInfo.end() != it) {
            //already know
            continue;
        } else {
            //This client know the group for the first time
            HandleNewGroup(grp);
        }
    }
}

void MainManager::HandleGrpMsgResp(grpmsgid_t grpmsgid, msgtime_t msgtime, grpid_t grpid, CppContent content) {
    mainWindow.HandleGrpMsgResp(grpmsgid, msgtime, grpid, content);
    WriteGrpMsgToDB(grpmsgid, msgtime, mainWindow.myid, grpid, content);
}

void MainManager::HandleReceivedGrpMsg(grpmsgid_t grpmsgid, msgtime_t msgtime, userid_t sender,
                                       grpid_t grpid, CppContent content)
{
    mainWindow.HandleGrpMsg(grpmsgid, msgtime, sender, grpid, content);
    WriteGrpMsgToDB(grpmsgid, msgtime, sender, grpid, content);
}

bool MainManager::WriteGrpMsgToDB(msgid_t grpmsgid, msgtime_t msgtime, userid_t sender, userid_t grpid, CppContent content) {
    SQLLEN length = content.size();
    SQLRETURN retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY, MAX_CONTENT_LEN,
                     0, content.data(), content.size(), &length);
    if (!SQL_SUCCEEDED(retcode)) {
        std::cerr << "Error in" << __PRETTY_FUNCTION__ << ": SQLBindParameter failed\n";
        return true;
    }
    return exec_sql("INSERT INTO grpmsg(grpmsgid, grpmsgtime, sender, togrp, content) VALUES(" + std::to_string(grpmsgid) + ',' +
             std::to_string(msgtime) + ',' + std::to_string(sender) + ',' + std::to_string(grpid) + ",?);", true);
}
