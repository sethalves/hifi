var localVideo;
var localStream;
var remoteVideo;
var peerConnection;
var uuid;
var serverConnection;

var sendChannel;


function pageReady() {
  uuid = createUUID();

  localVideo = document.getElementById('localVideo');
  remoteVideo = document.getElementById('remoteVideo');

  serverConnection = new WebSocket('ws://' + window.location.hostname + ':8889');
  serverConnection.addEventListener('message', function (event) {
    console.log('Message from server ', event.data);

    var signal = JSON.parse(event.data);

    if (signal.sdp) {
      if (signal.type == "answer") {
        console.log("it's an answer");

        peerConnection.setRemoteDescription(signal.sdp).then(function () {
          console.log("setRemoteDescription has been set");
        });
      }
    } else if (signal.ice) {
      console.log("ice server candidate from other side");
      peerConnection.addIceCandidate(signal.ice);
    } else {
      console.log("XXX unknown message");
    }

  });


  serverConnection.onerror = function (err) {
    console.log("serverConnection.onerror: " + JSON.stringify(err));
  }
  serverConnection.onclose = function () {
    console.log("serverConnection.onclose");
  }

  var constraints = {
    video: true,
    audio: true,
  };

  // if(navigator.mediaDevices.getUserMedia) {
  //   navigator.mediaDevices.getUserMedia(constraints).then(getUserMediaSuccess).catch(errorHandler);
  // } else {
  //   alert('Your browser does not support getUserMedia API');
  // }
}

// function getUserMediaSuccess(stream) {
//   localStream = stream;
//   localVideo.srcObject = stream;
// }


function onSdpError(err) {
  console.log("onSdpError called: " + JSON.stringify(err));
}

function start(isCaller) {
  console.log("start " + JSON.stringify(isCaller));

  if(isCaller) {
    var peerConnectionConfig = {
      'iceServers': [
        // {'urls': 'stun:stun.stunprotocol.org:3478'},
        {'urls': 'stun:stun.l.google.com:19302'},
      ]
    };

    peerConnection = new RTCPeerConnection(peerConnectionConfig);
    peerConnection.onconnectionstatechange = function (value) {
      // console.log("peerConnection.onconnectionstatechange: ");
      // console.dir(value);
      console.log("peerConnection.onconnectionstatechange: " + peerConnection.connectionState);
    }
    peerConnection.onsignalingstatechange = function (value) {
      // console.log("peerConnection.onsignalingstatechange: " + value.target.signalingState);
      // console.dir(value);
      console.log("peerConnection.onsignalingstatechange: " + peerConnection.signalingState);
    }
    peerConnection.onicecandidate = gotIceCandidate;
    peerConnection.ontrack = gotRemoteStream;
    // peerConnection.addStream(localStream);
    peerConnection.ondatachannel = function () {
      console.log("peerConnection.ondatachannel");
    }


    sendChannel = peerConnection.createDataChannel("sendChannel");
    sendChannel.onopen = function () {
      console.log("sendChannel.onopen");
      console.log("sendChannel state=" + sendChannel.readyState);
      sendChannel.send('Hi you!');
    }
    sendChannel.onclose = function () {
      console.log("sendChannel.onclose");
      console.log("sendChannel state=" + sendChannel.readyState);
    }
    sendChannel.onmessage = function (message) {
      console.log("sendChannel got message");
    }

    console.log("calling peerConnection.createOffer()");

    var offerAnswerConstraints = {
      optional: [],
      mandatory: {
        OfferToReceiveAudio: true,
        OfferToReceiveVideo: false
      }
    };

    peerConnection.createOffer(offerAnswerConstraints).then(function (sdp) {
      // console.log("my offer sdp: " + JSON.stringify(sdp));
      peerConnection.setLocalDescription(sdp).then(function () {
        console.log("local sdp is set.");

        var desc = JSON.stringify({'sdp': peerConnection.localDescription, 'uuid': uuid});
        console.log("sending description: " + desc);
        serverConnection.send(desc);
      });
    }); // .catch(onSdpError);

  }
}

function gotIceCandidate(event) {
  if (event.candidate != null) {
    console.log("sending ice candidate: " + event.candidate);
    serverConnection.send(JSON.stringify({'ice': event.candidate, 'uuid': uuid}));
  }
}

function gotRemoteStream(event) {
  console.log('got remote stream');
  remoteVideo.srcObject = event.streams[0];
}

function errorHandler(error) {
  console.log(error);
}

// Taken from http://stackoverflow.com/a/105074/515584
// Strictly speaking, it's not a real UUID, but it gets the job done here
function createUUID() {
  function s4() {
    return Math.floor((1 + Math.random()) * 0x10000).toString(16).substring(1);
  }

  return s4() + s4() + '-' + s4() + '-' + s4() + '-' + s4() + '-' + s4() + s4() + s4();
}
