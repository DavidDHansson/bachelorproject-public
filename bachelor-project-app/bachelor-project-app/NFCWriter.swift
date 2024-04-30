//
//  NFCWriter.swift
//  bachelor-project-app
//
//  Created by David Hansson on 02/04/2024.
//

import SwiftUI
import CoreNFC

// Taken from https://github.com/1998code/SwiftNFC
// (Modifications made)

public class NFCWriter: NSObject, ObservableObject, NFCNDEFReaderSessionDelegate {
    
    public var message = "test write"
    
    public var session: NFCNDEFReaderSession?
    
    public func write() {
        guard NFCNDEFReaderSession.readingAvailable else {
            print("Error")
            return
        }
        session = NFCNDEFReaderSession(delegate: self, queue: nil, invalidateAfterFirstRead: true)
        session?.alertMessage = "Hold your iPhone near the check in box..."
        session?.begin()
    }
    
    public func readerSession(_ session: NFCNDEFReaderSession, didDetectNDEFs messages: [NFCNDEFMessage]) {
    }

    public func readerSession(_ session: NFCNDEFReaderSession, didDetect tags: [NFCNDEFTag]) {
        if tags.count > 1 {
            let retryInterval = DispatchTimeInterval.milliseconds(500)
            session.alertMessage = "Please try again."
            DispatchQueue.global().asyncAfter(deadline: .now() + retryInterval, execute: {
                session.restartPolling()
            })
            return
        }
        
        let tag = tags.first!
        session.connect(to: tag, completionHandler: { (error: Error?) in
            if nil != error {
                session.alertMessage = "Unable to connect to box."
                session.invalidate()
                return
            }
            
            tag.queryNDEFStatus(completionHandler: { (ndefStatus: NFCNDEFStatus, capacity: Int, error: Error?) in
                guard error == nil else {
                    session.alertMessage = "Unable to query the status of box."
                    session.invalidate()
                    return
                }

                switch ndefStatus {
                case .readWrite:
                    let payload = NFCNDEFPayload.init(
                        format: .nfcWellKnown,
                        type: Data("T".utf8),
                        identifier: Data(),
                        payload: Data("\(self.message)".utf8)
                    )
                        
                    // payload = NFCNDEFPayload.wellKnownTypeURIPayload(string: "\(self.msg)")
                    let message = NFCNDEFMessage(records: [payload].compactMap({ $0 }))
                    tag.writeNDEF(message, completionHandler: { (error: Error?) in
                        if nil != error {
                            session.alertMessage = "Write to tag fail: \(error!)"
                        } else {
                            session.alertMessage = "Write \"\(self.message)\" to box successful."
                        }
                        session.invalidate()
                    })
                default:
                    session.alertMessage = "Failed."
                    session.invalidate()
                }
            })
        })
    }
    
    public func readerSessionDidBecomeActive(_ session: NFCNDEFReaderSession) {
    }

    public func readerSession(_ session: NFCNDEFReaderSession, didInvalidateWithError error: Error) {
        print("Session did invalidate with error: \(error)")
        self.session = nil
    }
}
