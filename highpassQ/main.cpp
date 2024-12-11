#include "mainwindow.h"

#include <QDir>
#include <QCoreApplication>
#include <QApplication>
#include "DatabaseManager.h"
#include "httpserver.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <netinet/in.h>
#include <unistd.h>

void initialize_openssl()
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl()
{
    EVP_cleanup();
}

SSL_CTX* create_context()
{
    const SSL_METHOD *method = SSLv23_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if(!ctx) {
        qDebug() << "Unable to create SSL context";
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void configure_context(SSL_CTX* ctx, const char* cert_file, const char* key_file)
{
    // ?¸ì¦ì„œ ë¡œë“œ
    if(SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0){
        qDebug() << "?¸ì¦ì„œ ë¡œë“œ ?˜¤ë¥?";
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    // ê°œì¸ ?‚¤ ë¡œë“œ
    if(SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0){
        qDebug() << "ê°œì¸ ?‚¤ ë¡œë“œ ?˜¤ë¥?";
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
//    const char* cert_file = "../server.crt";
//    const char* key_file = "../server.key";

//    initialize_openssl();
//    SSL_CTX* ctx = create_context();

//    configure_context(ctx, cert_file, key_file);

//    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
//    sockaddr_in addr;
//    addr.sin_family = AF_INET;
//    addr.sin_port = htons(4433);
//    addr.sin_addr.s_addr =INADDR_ANY;

//    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
//    listen(server_fd, 1);

//    while(true) {
//        struct sockaddr_in client_addr;
//        socklen_t len = sizeof(client_addr);
//        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);

//        qDebug() << "test001";
//        SSL *ssl = SSL_new(ctx);
//        qDebug() << "test002";
//        SSL_set_fd(ssl, client_fd);
//        qDebug() << "test003";
//        if(SSL_accept(ssl) <= 0){
//            qDebug() << "test error";
//            ERR_print_errors_fp(stderr);
//        }else{
//            qDebug() << "success ";
//            const char* reply = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, SSL!";
//            SSL_write(ssl, reply, strlen(reply));
//        }
//        qDebug() << "test004";

//        SSL_shutdown(ssl);  // ëª…ì‹œ? ?¸ ?—°ê²? ì¢…ë£Œ
//        SSL_free(ssl);
//        close(client_fd);
//    }
//    close(server_fd);
//    SSL_CTX_free(ctx);;
//    cleanup_openssl();


    QApplication a(argc, argv);

    //dbConnect
    DatabaseManager::instance().connectToDatabase(QString("../highpassQ/gotomars.db"));

    HttpServer server(DatabaseManager::instance());
    server.startServer(8080);


    MainWindow w;
    w.show();
    return a.exec();
}
