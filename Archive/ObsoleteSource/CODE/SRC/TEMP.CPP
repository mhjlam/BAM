(HandleMsg)	get local input - add to actionPool2
	main() - pEventMgr->PublishNext();

(Sync1)		send actionPool2 to remote
(Cycle)		process actionPool1
(Cycle)		cycle game
(Sync2)		receive remote actions for actionPool2
(Sync3)		swap actionPool2 w/ actionPool1
