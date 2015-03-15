/**
 * Tests whether an XMLHttpRequest has successfully finished loading.
 * @param {XMLHttpRequest} xhr The XHR object.
 */
function isSuccessful(xhr) {
  return xhr.readyState == 4 && xhr.status == 200;
}

function post(url, args, success, error) {
  var xmlhttp = new XMLHttpRequest();
  xmlhttp.onreadystatechange = function() {
    if (isSuccessful(xmlhttp))
      success(xmlhttp.responseText);
    else if (error != undefined)
      error(xmlhttp);
  };
  xmlhttp.open("POST", url, true);
  xmlhttp.setRequestHeader("Content-type",
                           "application/x-www-form-urlencoded");
  xmlhttp.send(args);
}

window.onload = function() {
  var start_btn = document.getElementById('start-btn');
  start_btn.onclick = function() {
    post('/api/start', '', function() {
      setTimeout(function() {
        post('/api/initial_status', '', function (response) {
          var commands = JSON.parse(response);
          var status_graph = document.getElementById('status-graph');

          // Remove children of status_graph.
          while (status_graph.firstChild) {
            status_graph.removeChild(status_graph.firstChild);
          }

          for (var i = 0; i < commands.length; ++i) {
            var li = document.createElement('li');
            li.setAttribute('id', 'command_' + commands[i].id);
            li.setAttribute('data-content', commands[i].content);
            status_graph.appendChild(li);
          }
        });
      }, 1000);

      setInterval(function() {
        post('/api/result', '', function(response) {
          var results = JSON.parse(response);
          for (var i = 0; i < results.length; ++i) {
            var li = document.getElementById('command_' + results[i].id);
            li.className = 'success';
          }
        }); 
      }, 1000);
    });
  }
}
