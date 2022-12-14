// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;
using Dasync.Collections;
using Jupiter.Implementation;
using Horde.Storage.Implementation;
using Microsoft.AspNetCore.TestHost;
using Microsoft.AspNetCore.Hosting;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using RestSharp;
using Serilog;
using Serilog.Core;
using Moq;

namespace Horde.Storage.FunctionalTests.GC
{
    [TestClass]
    public class GCBlobTests
    {
        private TestServer? _server;
        private HttpClient? _httpClient;

        private readonly BlobIdentifier object0id = new BlobIdentifier("0000000000000000000000000000000000000000");
        private readonly BlobIdentifier object1id = new BlobIdentifier("1111111111111111111111111111111111111111");
        private readonly BlobIdentifier object2id = new BlobIdentifier("2222222222222222222222222222222222222222");
        private readonly BlobIdentifier object3id = new BlobIdentifier("3333333333333333333333333333333333333333");
        private readonly BlobIdentifier object4id = new BlobIdentifier("4444444444444444444444444444444444444444");
        private readonly BlobIdentifier object5id = new BlobIdentifier("5555555555555555555555555555555555555555");
        private readonly BlobIdentifier object6id = new BlobIdentifier("6666666666666666666666666666666666666666");
        private Mock<IRestClient> _callistoBlobMock = null!;
        private IBlobStore? _blobStore;

        private readonly NamespaceId TestNamespace = new NamespaceId("test-namespace");

        [TestInitialize]
        public async Task Setup()
        {
            IConfigurationRoot configuration = new ConfigurationBuilder()
                // we are not reading the base appSettings here as we want exact control over what runs in the tests
                .AddJsonFile("appsettings.Testing.json", false)
                .AddEnvironmentVariables()
                .Build();

            Logger logger = new LoggerConfiguration()
                .ReadFrom.Configuration(configuration)
                .CreateLogger();


            _callistoBlobMock = GetCallistoMock(new CallistoReader.CallistoGetResponse(new TransactionEvent[]
            {
                new AddTransactionEvent("object0", "default", new[] {object0id}, identifier: 0, nextIdentifier: 1),
                new AddTransactionEvent("object2", "default", new[] {object2id}, identifier: 1, nextIdentifier: 2),
                new RemoveTransactionEvent("object2", "default", identifier: 2, nextIdentifier: 3),
                new AddTransactionEvent("object3", "default", new[] {object3id}, identifier: 3, nextIdentifier: 4),
                new AddTransactionEvent("object6", "default", new[] {object6id}, identifier: 4, nextIdentifier: 5)
            }.ToList(), Guid.Empty, 10));

            TestServer server = new TestServer(new WebHostBuilder()
                .UseConfiguration(configuration)
                .UseEnvironment("Testing")
                .UseSerilog(logger)
                .UseStartup<HordeStorageStartup>()
                .ConfigureTestServices(collection =>
                {
                    collection.AddSingleton<IBlobCleanup>(provider =>
                    {
                        IBlobStore blobStore = provider.GetService<IBlobStore>()!;
                        return new OrphanBlobCleanup(blobStore, new LeaderElectionStub(true), _callistoBlobMock.Object);
                    });

                    // make sure we have a memory blob store as we do some funky calling to it below
                    collection.AddSingleton<IBlobStore>(provider => ActivatorUtilities.CreateInstance<MemoryBlobStore>(provider));
                })
            );

            _httpClient = server.CreateClient();
            _server = server;

            _blobStore = server.Services.GetService<IBlobStore>()!;

            MemoryBlobStore memoryBlobStore = (MemoryBlobStore) _blobStore;
            byte[] emptyContents = new byte[0];
            await memoryBlobStore.PutObject(TestNamespace, emptyContents, object0id);
            await memoryBlobStore.PutObject(TestNamespace, emptyContents, object1id);// this is not in callisto
            await memoryBlobStore.PutObject(TestNamespace, emptyContents, object2id);
            await memoryBlobStore.PutObject(TestNamespace, emptyContents, object3id);
            await memoryBlobStore.PutObject(TestNamespace, emptyContents, object4id); // this is not in callisto
            await memoryBlobStore.PutObject(TestNamespace, emptyContents, object5id); // this is not in callisto
            await memoryBlobStore.PutObject(TestNamespace, emptyContents, object6id);

            // set all objects to be old, only the orphaned blobs should be deleted
            memoryBlobStore.SetLastModifiedTime(TestNamespace, object0id, DateTime.Now.AddDays(-2));
            memoryBlobStore.SetLastModifiedTime(TestNamespace, object1id, DateTime.Now.AddDays(-2));
            memoryBlobStore.SetLastModifiedTime(TestNamespace, object2id, DateTime.Now.AddDays(-2));
            memoryBlobStore.SetLastModifiedTime(TestNamespace, object3id, DateTime.Now.AddDays(-2));
            memoryBlobStore.SetLastModifiedTime(TestNamespace, object4id, DateTime.Now.AddDays(-2));
            memoryBlobStore.SetLastModifiedTime(TestNamespace, object5id, DateTime.Now.AddDays(-2));
            memoryBlobStore.SetLastModifiedTime(TestNamespace, object6id, DateTime.Now.AddDays(-2));
        }

        private Mock<IRestClient> GetCallistoMock(CallistoReader.CallistoGetResponse responseData)
        {
            // the callisto reader will do 2 requests to fetch the events (one to determine the generation and the other to actually fetch the objects)
            // after that we insert a empty response to indicate the end of the log was reached
            Mock<IRestClient> mock = new Mock<IRestClient> { DefaultValue = DefaultValue.Empty };

            
            Mock<IRestResponse<OrphanBlobCleanup.ListNamespaceResponse>> listNsResponse = new Mock<IRestResponse<OrphanBlobCleanup.ListNamespaceResponse>>();
            listNsResponse.Setup(_ => _.StatusCode).Returns(HttpStatusCode.OK);
            listNsResponse.Setup(_ => _.IsSuccessful).Returns(true);
            listNsResponse.Setup(_ => _.Data).Returns(new OrphanBlobCleanup.ListNamespaceResponse() { Logs = new [] {TestNamespace}});
            listNsResponse.Setup(_ => _.ResponseUri).Returns(new Uri("http://localhost"));
            
            mock.Setup(x =>
                x.ExecuteGetAsync<OrphanBlobCleanup.ListNamespaceResponse>(It.IsAny<IRestRequest>(),
                    It.IsAny<CancellationToken>())).ReturnsAsync(listNsResponse.Object).Verifiable();
            
            
            Mock<IRestResponse<CallistoReader.CallistoGetResponse>> response = new Mock<IRestResponse<CallistoReader.CallistoGetResponse>>();
            response.Setup(_ => _.StatusCode).Returns(HttpStatusCode.OK);
            response.Setup(_ => _.IsSuccessful).Returns(true);
            response.Setup(_ => _.Data).Returns(responseData);
            response.Setup(_ => _.ResponseUri).Returns(new Uri("http://localhost"));
            
            mock.Setup(x =>
                x.ExecuteGetAsync<CallistoReader.CallistoGetResponse>(It.IsAny<IRestRequest>(),
                    It.IsAny<CancellationToken>())).ReturnsAsync(response.Object).Verifiable();

            // when we do the second call to callisto with a offset we return a empty object indicating we have reached the end
            Mock<IRestResponse<CallistoReader.CallistoGetResponse>> responseSecondPage = new Mock<IRestResponse<CallistoReader.CallistoGetResponse>>();
            responseSecondPage.Setup(_ => _.StatusCode).Returns(HttpStatusCode.OK);
            responseSecondPage.Setup(_ => _.IsSuccessful).Returns(true);
            responseSecondPage.Setup(_ => _.Data).Returns(new CallistoReader.CallistoGetResponse(
                new List<TransactionEvent>(),
                responseData.Generation,
                responseData.CurrentOffset + 10 // set the offset to some arbitrary higher number to indicate we are at the end of the log
            ));
            responseSecondPage.Setup(_ => _.ResponseUri).Returns(new Uri("http://localhost"));
            mock.Setup(x => x.ExecuteGetAsync<CallistoReader.CallistoGetResponse>(
                    It.Is<IRestRequest>(request =>
                        request.Parameters.Any(parameter =>
                            parameter.Name == "offset" && parameter.Value!.ToString() != "0")),
                    It.IsAny<CancellationToken>()))
                .ReturnsAsync(responseSecondPage.Object).Verifiable();

            return mock;
        }

        [TestMethod]
        public async Task RunBlobCleanup()
        {
            OrphanBlobCleanup cleanup = new OrphanBlobCleanup(_blobStore!, new LeaderElectionStub(true), _callistoBlobMock.Object);
            List<NamespaceId> namespaces = await cleanup.ListNamespaces().ToListAsync();
            Assert.AreEqual(1, namespaces.Count);

            CancellationTokenSource cts = new CancellationTokenSource();
            List<RemovedBlobs> removedBlobs = await cleanup.Cleanup(cts.Token);
            Assert.AreEqual(4, removedBlobs.Count);
            
            BlobIdentifier[] expectedIdentifiers = {object1id, object2id, object4id, object5id};
            foreach (RemovedBlobs removedBlob in removedBlobs)
            {
                Assert.IsTrue(expectedIdentifiers.Contains(removedBlob.BlobIdentifier));
            }
            
            _callistoBlobMock.Verify();
            _callistoBlobMock.VerifyNoOtherCalls();
        }
    }
}
