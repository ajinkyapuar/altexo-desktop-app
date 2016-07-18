#include "manager.hpp"
#include "contact.hpp"
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>

Manager::Manager() : m_wsClient(NULL), m_sensor(NULL), m_sdk(NULL) {
  std::cout << "Manager constructor" << std::endl;
}

Manager::~Manager() {}

void Manager::initSensor(AlSensorCb *sensorCb) {
  boost::filesystem::path lib_path("/home/xors/workspace/QT/altexo/build/");
  std::cout << "Loading sensor plugin" << std::endl;
  m_sensor = boost::dll::import<AlSensorAPI>(
      lib_path / "libal_kinect_1.so", "plugin",
      boost::dll::load_mode::append_decorations);

  m_sensor->init(sensorCb);

  // tread requesting new sensor frame every 30 milliseconds
  boost::thread m_frameThread = boost::thread(&Manager::frameThread, this);
}

void Manager::frameThread() {
  // TODO: add finishing when destroying object
  while (true) {
    m_sensor->requestNewFrame();
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000 / 30));
  }
}

void Manager::initWsConnection(AlWsCb *alWsCb) {
  boost::filesystem::path lib_path("/home/xors/workspace/QT/altexo/build/");
  std::cout << "Loading ws plugin" << std::endl;
  m_wsClient =
      boost::dll::import<AlWsAPI>(lib_path / "libws_client.so", "plugin",
                                  boost::dll::load_mode::append_decorations);

  m_wsClient->init(alWsCb);
}

void Manager::initSdk() {
  boost::filesystem::path lib_path("/home/xors/workspace/QT/altexo/build/");
  std::cout << "Loading sdk plugin" << std::endl;
  m_sdk =
      boost::dll::import<AlSdkAPI>(lib_path / "libaltexo_sdk.so", "plugin",
                                   boost::dll::load_mode::append_decorations);

  m_sdk->init(this);
  // newMessageToSdk.connect(
  //     boost::bind(&AlSdkAPI::onMessageFromPeer, m_sdk, _1, _2));
}

void Manager::onWsMessageCb(std::vector<char> msg) {
  std::string msgStr(msg.begin(), msg.end());
  if (m_debug) {
    std::cout << "=== ws message ===" << std::endl;
    std::cout << msgStr << std::endl;
  }
  boost::property_tree::ptree pt;
  std::stringstream ss(msgStr);
  boost::property_tree::read_json(ss, pt);
  std::string action = pt.get<std::string>("action");
  if (action == "logged_in") {
    m_id = pt.get<std::string>("data.id");
  } else if (action == "update_user_list") {
    // updating contact list
    contactList.clear();
    for (auto &item : pt.get_child("data")) {
      CONTACT ct;
      ct.name = item.second.get<std::string>("name");
      ct.id = item.second.get<std::string>("id");
      contactList.push_back(ct);
    }
  } else if (action == "message_from_peer") {
    onMessageFromPeer(pt);
  }
}

void Manager::initConnection(std::string mode) {
  if (m_wsClient != NULL) {
    std::ostringstream stream;
    boost::property_tree::ptree pt;
    pt.put("mode", mode);
    boost::property_tree::write_json(stream, pt, false);
    std::string strJson = stream.str();
    std::vector<char> strVec(strJson.begin(), strJson.end());
    std::vector<char> peerIdVec(m_peerId.begin(), m_peerId.end());
    m_wsClient->sendMessageToPeer(peerIdVec, strVec);
  }
}

void Manager::onMessageFromPeer(boost::property_tree::ptree msgPt) {
  std::string senderIdStr = msgPt.get<std::string>("data.sender_id");
  std::string messageStr = msgPt.get<std::string>("data.message");

  boost::property_tree::ptree jsonMsg;
  std::stringstream ss(messageStr);
  boost::property_tree::read_json(ss, jsonMsg);

  if (m_debug) {
    std::cout << messageStr << std::endl;
  }

  boost::optional<bool> callAccepted =
      jsonMsg.get_optional<bool>("callAccepted");

  boost::optional<bool> isCall = jsonMsg.get_optional<bool>("call");

  // this is the very first contact we will store peer id
  if (m_peerId == "-1") {
    m_peerId = senderIdStr;
    if (m_videoDeviceName != "") {
      // m_sdk->setDesiredDataSource(AlManagerInterface::CAMERA);
    } else {
      // **
      // * INITIATE MODE
      // *
      // std::cout << "intiate" << std::endl;
      //
      // std::vector<char> peerIdVec(m_peerId.begin(), m_peerId.end());

      // initConnection("hologram");
      initConnection("audio+video");
      // // Q_EMIT requestNewFrameSignal();
      // // // TODO: take a look later
      // // //            QTimer *timer = new QTimer(this);
      // // //            connect(timer, SIGNAL(timeout()), this,
      // // //            SLOT(timeoutSlot()));
      // // //            timer->start(1000);
    }
  }

  if (callAccepted) {
    if (m_debug) {
      std::cout << "callAccepted" << std::endl;
    }
    //     Q_EMIT ConnectToPeerSignal(
    //         jsonObj["data"].toObject()["sender_id"].toString());
  } else if (isCall) {
    if (m_debug) {
      std::cout << "isCall" << std::endl;
    }
    // TODO implement accept call functionality
    std::ostringstream stream;
    boost::property_tree::ptree msgToSendPt;
    msgToSendPt.put("callAccepted", true);
    boost::property_tree::write_json(stream, msgToSendPt, false);
    std::string strJson = stream.str();
    if (m_debug) {
      // std::cout << strJson << std::endl;
    }
    std::vector<char> strVec(strJson.begin(), strJson.end());
    std::vector<char> peerIdVec(m_peerId.begin(), m_peerId.end());
    m_wsClient->sendMessageToPeer(peerIdVec, strVec);

    // TODO only for initiating side
    // m_sdk->initializePeerConnection();
  } else {
    if (m_debug) {
      std::cout << "================" << std::endl;
      std::cout << "sdp or candidate" << std::endl;
      std::cout << "================" << std::endl;
    }
    boost::optional<std::string> sdp = jsonMsg.get_optional<std::string>("sdp");
    if (sdp) {
      if (m_debug) {
        std::cout << "sdb received" << std::endl;
      }
      m_remoteSdp = messageStr;
    } else {
      if (m_debug) {
        std::cout << "ice candidate received" << std::endl;
      }
      m_remoteCandidates.push(messageStr);
    }
    handleMessages();
  }
}

void Manager::onSdp(std::vector<char> sdp) {
  if (m_debug) {
    std::cout << "Manager::onSdp" << std::endl;
  }
  m_localSdp = std::string(sdp.begin(), sdp.end());
  handleMessages();
}
void Manager::onCandidate(std::vector<char> candidate) {
  if (m_debug) {
    std::cout << "Manager::onCandidate" << std::endl;
  }
  m_localCandidates.push(std::string(candidate.begin(), candidate.end()));
  handleMessages();
}

void Manager::handleMessages() {
  std::vector<char> peerIdVec(m_peerId.begin(), m_peerId.end());
  if (!m_sentLocalSdp && m_localSdp != "") {
    std::vector<char> localSdpVec(m_localSdp.begin(), m_localSdp.end());
    m_wsClient->sendMessageToPeer(peerIdVec, localSdpVec);
    m_sentLocalSdp = true;
  }
  if (!m_sentRemoteSdp && m_remoteSdp != "") {
    std::vector<char> remoteSdpVec(m_remoteSdp.begin(), m_remoteSdp.end());
    m_sdk->setRemoteSdp(remoteSdpVec);
    m_sentRemoteSdp = true;
  }
  if (m_sentLocalSdp && m_sentRemoteSdp) {
    while (!m_localCandidates.empty()) {
      std::string candidate = m_localCandidates.front();
      m_localCandidates.pop();
      std::vector<char> candidateVec(candidate.begin(), candidate.end());
      m_wsClient->sendMessageToPeer(peerIdVec, candidateVec);
    }
    while (!m_remoteCandidates.empty()) {
      std::string candidate = m_remoteCandidates.front();
      m_remoteCandidates.pop();
      std::vector<char> candidateVec(candidate.begin(), candidate.end());
      m_sdk->setRemoteIceCandidate(candidateVec);
    }
  }
}
