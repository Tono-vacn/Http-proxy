FROM ubuntu:latest
RUN apt-get update && \
    apt-get install -y g++ && \
    apt-get install -y make && \
    apt-get install -y libboost-all-dev && \
    apt-get clean
RUN mkdir /var/log/erss
RUN touch /var/log/erss/proxy.log
RUN mkdir /code
ADD . /code
WORKDIR /code
RUN ["chmod", "+x", "run.sh"]
EXPOSE 12345