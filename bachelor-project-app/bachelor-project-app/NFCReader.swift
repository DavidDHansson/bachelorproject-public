//
//  NFCReader.swift
//  bachelor-project-app
//
//  Created by David Hansson on 02/04/2024.
//

import SwiftUI
import CoreNFC

// Taken from https://github.com/1998code/SwiftNFC
// (Modifications made)

class NFCReader: NSObject, NFCNDEFReaderSessionDelegate {
    
    var session: NFCNDEFReaderSession?
    
    func read() {
        guard NFCNDEFReaderSession.readingAvailable else {
            print("Error")
            return
        }
        session = NFCNDEFReaderSession(delegate: self, queue: nil, invalidateAfterFirstRead: true)
        session?.alertMessage = "Hold your iPhone near the check in box..."
        session?.begin()
    }
    
    func readerSession(_ session: NFCNDEFReaderSession, didDetectNDEFs messages: [NFCNDEFMessage]) {
        DispatchQueue.main.async { [weak self] in
            let rawMessage = messages.map {
                $0.records.map {
                    String(decoding: $0.payload, as: UTF8.self)
                }.joined(separator: "\n")
            }.joined(separator: " ")
            
            guard let url = URL(string: rawMessage.trimmingCharacters(in: .controlCharacters)),
                  url.pathComponents.first == "location" else {
                session.alertMessage = "Error in reading location. Please try again or contact staff."
                return
            }
            
            let id = url.lastPathComponent
            
            session.alertMessage = "Successfully read location #\(id) check in box"
            
            Task {
                await self?.handleScan(location: Int(id))
            }
        }
    }
    
    func readerSessionDidBecomeActive(_ session: NFCNDEFReaderSession) {
    }
    
    func readerSession(_ session: NFCNDEFReaderSession, didInvalidateWithError error: Error) {
        print("Session did invalidate with error: \(error)")
        self.session = nil
    }
    
    private func handleScan(location: Int?) async {
        // Create URL
        guard let url = URL(string: "") else {
            print("Invalid URL")
            return
        }
        
        guard let location else {
            print("Invalid location id")
            return
        }
        
        // Create URLRequest
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        
        // Set headers
        request.setValue("Bearer ", forHTTPHeaderField: "Authorization")
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        
        // Set request body
        let parameters: [String: Any] = [
            "userId": UserManager.id,
            "locationId": location,
            "invalidate": true
        ]
        
        // Serialize
        do {
            let jsonData = try JSONSerialization.data(withJSONObject: parameters)
            request.httpBody = jsonData
        } catch {
            print("Error serializing JSON: \(error)")
            return
        }
        
        do {
            // Perform the request and get the response
            let (_, response) = try await URLSession.shared.data(for: request)
            
            // Check for HTTP response
            guard let httpResponse = response as? HTTPURLResponse else {
                print("Invalid response")
                return
            }
            
            print("Processed user with id \(UserManager.id)")
            print("Status code: \(httpResponse.statusCode)")
        } catch {
            print("Error: \(error)")
        }
    }
    
}
