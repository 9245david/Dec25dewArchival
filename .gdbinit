define cls
shell clear
end
document cls
Clears the screen with a simple command.
end
define print_destip
set $sockfd = (int)$arg0
struct sockaddr_in sa;

int len = sizeof(sa);

getsockname(sockfd, (struct sockaddr *)&sa, &len);

printf("本连接的本地IP：%s", inet_ntoa(sa.sin_addr))
end
document print_destip
print sockfd dest ip
