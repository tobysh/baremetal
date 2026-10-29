import assert from "node:assert/strict";
import {createHash, randomBytes} from "node:crypto";
import {readFile} from "node:fs/promises";
import {MongoClient, ObjectId} from "mongodb";
const base = process.argv[2] ?? "http://localhost:3000";
const client = new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017");
const db = client.db(process.env.MONGODB_DB ?? "baremetal");
const id = new ObjectId(), token = randomBytes(32).toString("hex");
const tokenHash = createHash("sha256").update(token).digest("hex");
const exerciseId = "first-steps/logical-operators";
const code = await readFile("content/first-steps/10-logical-operators/solution.c","utf8");
const headers = {Origin:base,"Content-Type":"application/json",Cookie:`bm_session=${token}`};
async function json(path: string, method: string, body?: unknown) {
 const res = await fetch(base+path,{method,headers,body:body ? JSON.stringify(body) : undefined});
 const data = await res.json(); assert(res.ok,JSON.stringify(data)); return data;
}
try {
 await client.connect();
 await db.collection("users").insertOne({_id:id,email:`workspace-check-${id}@example.test`,name:"Workspace check",experience:"beginner",role:"learner",createdAt:new Date(),sessionVersion:0});
 await db.collection("sessions").insertOne({userId:id,tokenHash,sessionVersion:0,expiresAt:new Date(Date.now()+60000),createdAt:new Date()});
 assert.equal((await json('/api/run','POST',{exerciseId,code,stdin:'2000',mode:'run'})).stdout,'2000 is a leap year\n');
 await json('/api/progress','PUT',{exerciseId,code});
 assert.equal((await db.collection('progress').findOne({userId:id,exerciseId}))?.code,code);
 // Test anonymously to avoid changing any real learner XP or creating award fixtures.
 const res = await fetch(base+'/api/run',{method:'POST',headers:{Origin:base,'Content-Type':'application/json'},body:JSON.stringify({exerciseId,code,mode:'test'})});
 const result = await res.json(); assert(res.ok); assert.equal(result.allPassed,true); assert.equal(result.results.length,4);
 await db.collection('tutor_threads').insertOne({userId:id,lessonId:exerciseId,messages:[{role:'user',content:'help',at:new Date()}]});
 await json('/api/tutor?exerciseId='+encodeURIComponent(exerciseId),'DELETE');
 assert.equal((await db.collection('progress').findOne({userId:id,exerciseId}))?.tutorUsed,true);
 assert.equal(await db.collection('tutor_threads').countDocuments({userId:id}),0);
 console.log('Live leap-year Run, Test, signed-in autosave and sticky tutor-use checks passed.');
} finally {
 await Promise.all(['users','sessions','progress','tutor_threads'].map((c)=>db.collection(c).deleteMany(c==='users'?{_id:id}:{userId:id})));
 await db.collection('rate_limits').deleteMany({key:{$regex:id.toHexString()}});
 await client.close();
}
